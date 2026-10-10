# Контроль запуска через штатные библиотеки

**Наблюдение v0.1: SD_BEGIN PASS, обнаружение W5100 FAIL (NONE), DHCP
не запускался. v0.2 ETH_ONLY: передан один незавершённый прогон и один
STARTUP PASS (W5100 обнаружен, DHCP успешен, ненулевой IP, терминальный PASS).
SD_THEN_ETH v0.2: SD_BEGIN PASS, обнаружение W5100 FAIL (NONE), DHCP
не запускался.** Это контроль старта UNO/W5100/SD
для локализации отказа перед продолжением TEST-09. Он не заменяет критерии
TEST-07/08/09 и не подтверждает обмен UDP или сохранность файлов.

v0.2 ждёт выбора режима от логгера после `READY test=LIBRARY_STARTUP fw=0.2`.
В одном бинарнике доступны `SD_THEN_ETH` и `ETH_ONLY`. Команда отправляется
автоматически, в UART и JSONL фиксируется выбранный режим. При отсутствии
команды в течение 10 секунд получается MODE_TIMEOUT FAIL. Неверная или
слишком длинная команда даёт INVALID_MODE/COMMAND_OVERFLOW FAIL. При этих
отказах SPI-вызовы не выполняются.

В режиме `SD_THEN_ETH` выполняются только публичные вызовы библиотек:

1. Оба CS выставляются HIGH до переключения пинов в OUTPUT.
2. `SD.begin(4)` проверяет инициализацию карты, FAT и корня.
3. `Ethernet.init(10)` задаёт CS, затем однократно
   `Ethernet.begin(mac, 6000, 1000)` запускает драйвер и DHCP.
4. Печатаются обнаруженный чип, результат DHCP и IP. После этого только
   UART heartbeat раз в 5 секунд: дополнительных сетевых/SD операций нет.

В режиме `ETH_ONLY` CS D4 — HIGH, `SD.begin` не вызывается; вместо него
выводится `SD_BEGIN status=SKIP`. Карта может быть вставлена или извлечена
согласно подготовке конкретного контроля. UART не подтверждает её физическое положение.
Ethernet использует те же MAC, CS, библиотеку и параметры DHCP. RESULT
содержит `mode=ETH_ONLY`; его PASS относится только к старту Ethernet и
не является PASS совместной работы SD + Ethernet.

Приложение не использует raw SPI, RTR, индексированные EthernetClient,
принудительное закрытие сокетов, дополнительные idle clocks, повторные попытки
или изменение частоты SPI. Файлы не создаются и не удаляются. Библиотеки
самостоятельно управляют SPI в своих вызовах.

## Почему этот контроль нужен

В SD 1.3.0 `SD.begin(4)` внутри вызывает
`card.init(SPI_HALF_SPEED, 4)`, затем `volume.init` и `root.openRoot`.
Само изменение имени API не устраняет причину отказа. SD первой уже
инициализируется в TEST-09 v0.2; это не новое исправление её порядка запуска.
Контроль исключает прикладные SPI-диагностики и файловую нагрузку, чтобы
проверить обычную последовательность вызовов библиотек.

`Ethernet.init(10)` только выбирает CS. Обнаружение W5100 выполняется в
`Ethernet.begin`. Если после него `hardwareStatus()` сообщает NONE,
`rc=0` не доказывает проблему сервера DHCP: библиотека вернулась до DHCP.
Скетч в этом случае выводит `DHCP status=NOT_ATTEMPTED` и FAIL этапа HARDWARE.
`begin_elapsed_ms` включает обнаружение чипа и DHCP, а не только DHCP.

Параметры 6000/1000 ms задают ожидания DHCP, но не являются аппаратным
watchdog для всех внутренних SPI-вызовов. Host ограничивает запись UART
30 секундами. Последняя строка STEP показывает, перед каким вызовом оборвался
протокол наблюдения; сама по себе она не устанавливает причину зависания.

## Сборка и загрузка — PowerShell 7

Карта и Ethernet-кабель остаются подключёнными. Монитор Arduino должен быть
закрыт. Используются уже установленные SPI 1.0 из AVR, SD 1.3.0 и Ethernet
2.0.2; менять библиотеки для этого контроля не требуется.

Из корня репозитория, на ветке `feature/w5100-test09-ethernet-sd`:

```powershell
git pull --ff-only
if ($LASTEXITCODE -ne 0) { throw 'git pull failed' }
$ref = (Resolve-Path '.\labs\01-W5100-Ethernet\TEST-09_Ethernet_SD_Integration\reference\LibraryStartup').Path
$buildRef = Join-Path (Get-Location).Path 'build\LibraryStartup'
$null = New-Item -ItemType Directory -Path $buildRef -Force
arduino-cli compile --fqbn arduino:avr:uno --warnings all --build-path $buildRef --json $ref |
    Set-Content (Join-Path $buildRef 'compile.json') -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; do not upload' }
$compiled = Get-Content (Join-Path $buildRef 'compile.json') -Raw | ConvertFrom-Json
if (-not $compiled.success) { throw 'Compiler did not report success' }
$platform = $compiled.builder_result.build_platform
if ($platform.id -ne 'arduino:avr' -or $platform.version -notin @('1.8.6','1.8.8')) {
    throw 'Expected AVR core 1.8.6 or 1.8.8'
}
foreach ($expected in @(@('SPI','1.0'), @('SD','1.3.0'), @('Ethernet','2.0.2'))) {
    $selected = @($compiled.builder_result.used_libraries | Where-Object name -eq $expected[0])
    if ($selected.Count -ne 1 -or $selected[0].version -ne $expected[1]) {
        throw "Unexpected library: $($expected[0])"
    }
}
$sections = $compiled.builder_result.executable_sections_size
$flash = ($sections | Where-Object name -eq 'text').size
$sram = ($sections | Where-Object name -eq 'data').size
if ($null -eq $flash -or $null -eq $sram -or $flash -gt 29000 -or $sram -gt 1536) {
    throw 'Reference build memory gate failed'
}
Write-Host "BUILD PASS / Flash=$flash/32256 / static SRAM=$sram/2048 / AVR=$($platform.version)" -ForegroundColor Green
arduino-cli upload --fqbn arduino:avr:uno --port COM4 --input-dir $buildRef $ref
if ($LASTEXITCODE -ne 0) { throw 'Upload failed' }
```

Загружается бинарник из только что измеренного каталога `build\LibraryStartup`.
Сборка v0.2 проверена на AVR 1.8.6: **18 534 B Flash, 1 109 B static SRAM**.
Оба режима выполняются в этом же измеренном бинарнике. Историческая v0.1:
18 090 B Flash и 1 085 B static SRAM, включая подтверждённую сборку на AVR 1.8.8.
Четыре предупреждения unused parameter относятся к `new.cpp` AVR core;
предупреждений из скетча нет. На AVR 1.8.8 размеры нужно измерить заново.
Свободная SRAM в UART — выборочные значения, не измерение максимального
стека внутри библиотек или ISR.

## Ограниченная запись UART

Существующий логгер TEST-07 записывает все строки, даже если его специализированный
парсер не распознаёт поля этого скетча. По `status=PASS/FAIL` он окрашивает
текст консоли; JSONL содержит исходные строки без цветовых кодов.
Логгер не выносит собственный автоматический вердикт.

```powershell
$watch = '.\labs\01-W5100-Ethernet\TEST-07_Network_Robustness\Watch-NetworkEvents.ps1'
$logsRef = Join-Path $ref 'runs'
```

Пример подготовки выполненного контроля `SD_THEN_ETH` с картой на месте.
Полное отключение питания
перед ним задаёт известное начальное состояние карты: сброс UNO через DTR
сам по себе не отключает питание microSD. Это условие контроля начального
состояния, а не предложенное исправление отказа.

```powershell
$null = Read-Host 'Отключи всё питание UNO/Shield. При отключённом питании вставь ту же microSD. Ethernet-кабель оставь подключённым. Затем нажми Enter'
$null = Read-Host 'Подключи USB к UNO. Закрой монитор Arduino. Затем нажми Enter'
& $watch -SerialPort COM4 -DurationSeconds 30 -OutputDirectory $logsRef -LibraryStartupMode SD_THEN_ETH
```

Для контроля Ethernet без вызова SD.begin на той же прошивке используется
`-LibraryStartupMode ETH_ONLY` с подготовленным положением карты. Один
прогон, а также сравнение разных начальных состояний карты, не устанавливают
причину перемежающегося отказа. Эти режимы позволяют различить обнаружение
W5100 после SD.begin и без этого вызова при физически вставленной карте.

Путь `$ref` абсолютный: .NET и PowerShell используют один каталог логов.
Открытие COM с DTR может сбросить UNO. Карту и кабель не трогать; клавиши не
нужны. В логе должны быть один BOOT этой прошивки, подтверждение MODE и один терминальный RESULT
того же запуска. Если BOOT пропущен, появился повторный BOOT, нет RESULT или
есть FAIL, аппаратный PASS по этому наблюдению не устанавливается.

Ограниченный **SD_THEN_ETH STARTUP PASS** требует одновременно: SD_BEGIN PASS,
HARDWARE chip=W5100 PASS, DHCP rc=1 PASS, ненулевой IP PASS и
RESULT status=PASS stage=COMPLETE mode=SD_THEN_ETH.
**ETH_ONLY STARTUP PASS** требует SD_BEGIN SKIP, MODE name=ETH_ONLY,
HARDWARE chip=W5100 PASS, DHCP rc=1 PASS, ненулевой IP PASS и
RESULT status=PASS stage=COMPLETE mode=ETH_ONLY. Это не критерий PASS SD.
SD_BEGIN FAIL не различает отказ CMD0,
FAT или открытия корня: публичный API возвращает один bool. HARDWARE FAIL
означает отказ обнаружения ожидаемого чипа библиотекой в этой последовательности,
но не устанавливает аппаратную причину. Даже полный STARTUP PASS не
доказывает корректность чередования записи SD и UDP в TEST-09.

## Проверенные первичные источники

- [Arduino Ethernet Shield V1](https://www.arduino.cc/en/Main/ArduinoEthernetShieldV1):
  общий SPI, Ethernet CS D10 и SD CS D4.
- [SD 1.3.0: SDClass::begin](https://github.com/arduino-libraries/SD/blob/1.3.0/src/SD.cpp):
  фактическая последовательность card/volume/root.
- [Ethernet 2.0.2: init, begin, hardwareStatus](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/Ethernet.cpp)
  и [драйвер W5100](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/utility/w5100.cpp).
- [Adafruit: Serving Files over Ethernet](https://learn.adafruit.com/arduino-ethernet-sd-card/serving-files-over-ethernet):
  опубликованный пример SD → Ethernet. Он рассчитан на W5500;
  аналогия порядка вызовов не подтверждает работоспособность данного W5100.

## Что дают обсуждение и документация Shield Rev2

[Обсуждение на Arduino.ru](https://arduino.ru/forum/programmirovanie/ethernet-shield-c-sd-kartoi-ne-pishet-na-kartu)
содержит разные случаи и версии библиотек. В сообщениях 23/26 участник сообщает
об успехе после переноса инициализации в setup; сообщение 27 уже ставит под
сомнение необходимость ручных SWITCH_TO, поскольку дальнейшие вызовы работают
без них. В сообщении 29 описан похожий симптом: Ethernet работает без карты,
но зависает со вставленной картой без вызова SD.begin. Это аналогия наблюдения,
а не установленная причина нашего отказа или подтверждённое исправление.

[Shield Rev2](https://docs.arduino.cc/hardware/ethernet-shield-rev2/)
использует W5500, что явно указано в
[официальных характеристиках](https://store.arduino.cc/products/arduino-ethernet-shield-2).
Его принцип общего SPI и раздельных CS применим, но схема Rev2 не является
схемой исследуемого W5100 shield. В документации W5100 V1 также указано:
Ethernet CS D10, SD CS D4; HIGH снимает выбор устройства. Работа обоих устройств
в одной программе предусмотрена, а SPI-доступ к ним чередуется библиотеками.
D53 относится к аппаратному SS Mega и не заменяет D10 на UNO.

В выбранных официальных SD 1.3.0 и Ethernet 2.0.2 есть beginTransaction и
endTransaction; SD снимает CS в chipSelectHigh, а Ethernet устанавливает свои
SPISettings при начале транзакции. В нашем скетче оба CS уже OUTPUT/HIGH,
инициализация однократная в setup, ручного удержания CS в LOW нет. Поэтому
перенос форумных макросов сам по себе не обосновывает исправление.

## Сверка установленных библиотек без обращения к плате

Таблица used_libraries подтверждает выбор версий и каталогов, но не проверяет
изменения внутри установленных файлов. `Verify-LibrarySources.ps1` читает
фактически выбранные каталоги из свежего compile.json и сравнивает 34 файла
с `LibrarySourceManifest.json`: SPI 1.0 — 3, Ethernet 2.0.2 — 16, SD 1.3.0 — 15.
Проверяются library.properties и файлы src; дополнительные исходники/заголовки
в src тоже вызывают FAIL. Библиотеки, плата и прошивка не изменяются; JSON-отчёт
сохраняется только локально в runs. Полный отчёт содержит локальные пути и не
нужен для публикации.

Эталон построен из официальных репозиториев и тегов. До вычисления SHA-256
содержимое каждого файла проверено по Git blob SHA-1. SPI из AVR 1.8.6 и 1.8.8
совпадает побайтно; идентификаторы исходных коммитов и blob приведены в manifest.
Для сравнения текст читается как UTF-8, удаляется ведущий BOM, CRLF/CR заменяются
на LF и удаляются завершающие LF. Пробелы внутри файла и остальные символы не
нормализуются.

Из корня репозитория, без загрузки на UNO:

```powershell
$ref = (Resolve-Path '.\labs\01-W5100-Ethernet\TEST-09_Ethernet_SD_Integration\reference\LibraryStartup').Path
$buildRef = Join-Path (Get-Location).Path 'build\LibraryStartup'
$null = New-Item -ItemType Directory -Path $buildRef -Force
arduino-cli compile --fqbn arduino:avr:uno --build-path $buildRef --json $ref |
    Set-Content (Join-Path $buildRef 'compile.json') -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; source audit stopped' }
& "$ref\Verify-LibrarySources.ps1" -CompileJson (Join-Path $buildRef 'compile.json')
```

Зелёный `LIBRARY SOURCE AUDIT PASS` требует совпадения всех 34 файлов,
ожидаемых версий и выбора SPI из выбранного AVR core. Красный FAIL означает
отличие, отсутствие файла, лишний исходник или неверный выбор библиотеки.
Это проверка тождественности исходников после объявленной нормализации; она
не доказывает отсутствие ошибок в самих официальных библиотеках, не проверяет
весь AVR core/параметры компилятора/байты загруженной прошивки и не является
аппаратным PASS. На компьютере оператора результат пока PENDING.

Программная проверка помощника: 34/34 официальных файла в локальной среде дали
PASS. На изолированных копиях изменение инструкции SPI, удаление заголовка SD
и добавление исходника Ethernet были обнаружены с FAIL. Реальные установленные
библиотеки при этой отрицательной проверке не менялись.
