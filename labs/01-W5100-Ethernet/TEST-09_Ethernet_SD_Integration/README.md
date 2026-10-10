# TEST-09 — Ethernet + microSD integration

**BUILD VERIFIED / first hardware FAIL (CARD_INIT); diagnosis PENDING.** TEST-08 passed its 15 checks after full
power-off and reported a second ordinary-repeat PASS; both earlier failures
remain documented. TEST-09 exercises the same UNO/W5100/card with real UDP
traffic passing through a file on SD. It has no hardware PASS until the
operator supplies a completed load result. The first run failed at CARD_INIT
(error_code=1/error_data=255), before file creation, DHCP or UDP.
[Actual result](RESULT_2026-10-10.md). TEST-07 DhcpOutage remains deferred.

## Что проверяется

PowerShell отправляет пакет с уникальным токеном запуска и номером. UNO
записывает его в собственный `T09CHECK.BIN`, выполняет sync/close, сбрасывает
файловый кэш, повторно открывает файл и читает байты с SD. Только эти прочитанные
байты уходят в UDP-ответ. Host проверяет адрес отправителя, длину и каждый байт.
CRC16 дополнительно сверяется на MCU и в итоговом отчёте. Размеры циклически:
**16, 32, 64, 128 B**; SPI D10/D4 всегда выбирает только одно устройство.

После операций SD выдаётся idle-байт FF при обоих CS=HIGH перед обращением к
W5100 через Ethernet library. Состояние RTR проверяется до нагрузки и двумя
чтениями после неё. Это проверка чередования UDP и SD в одном приложении;
параллельность SPI-транзакций не предполагается. TCP+SD, HTTP-сервер и
прикладной сетевой регистратор этим этапом не сертифицируются.

## Подготовка и сборка — Windows / PowerShell 7

UNO ATmega328P, W5100 CS=D10, SD CS=D4, SPI ICSP/D11–D13, UART 115200.
Та же карта FAT16/FAT32 остаётся вставленной; кабель Ethernet подключён к
обычной LAN с DHCP. PC должен иметь маршрут до выданного UNO IPv4.
Изменять домашний роутер, DHCP timeout, Wi-Fi или firewall для запуска не нужно.
Закройте Arduino Serial Monitor и другие владельцы COM4.

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git switch feature/w5100-test09-ethernet-sd
git pull --ff-only
$test9 = ".\labs\01-W5100-Ethernet\TEST-09_Ethernet_SD_Integration"
arduino-cli lib install "Ethernet@2.0.2" "SD@1.3.0"
& "$test9\Build-Test09.ps1" -UploadPort COM4
```

Helper принимает AVR 1.8.6 или 1.8.8, проверяет версии обеих библиотек, реальные
Flash/SRAM и SHA256 трёх исходников/HEX. Он загружает именно измеренный HEX.
Локальная реальная сборка: Arduino CLI 1.3.1 / AVR 1.8.6, **25 236 B Flash /
1 440 B статической SRAM**. Оператор подтвердил те же размеры с AVR 1.8.8
и успешную загрузку на COM4; локальные HEX/source hash он пока не предоставил.

## Первый прогон — без ручных действий

```powershell
& "$test9\Test-EthernetSD.ps1" -SerialPort COM4 -DurationSeconds 60
```

Host ждёт один BOOT/READY и посылает RUN с новым токеном. Прошивка сначала
инициализирует SD и эксклюзивно создаёт файл, затем получает DHCP и открывает
UDP :5001. IP читается из UART — прежний адрес не подставляется вручную.
После NET начинаются 60 секунд нагрузки. Пауза между проверками 100 ms,
поэтому частота не превышает примерно 10 запросов/s, а фактическая зависит от
SD и сети. Повторных отправок после TIMEOUT нет. При первой ошибке прогон FAIL.
В конце host посылает STOP с тем же токеном и ждёт итоговый RESULT.

DurationSeconds допустим от 30 до 600; начните с 60. Подготовка ограничена
30 s, ожидание каждого UDP-ответа по умолчанию 2000 ms, итогового RESULT — 10 s.
SD и DHCP вызовы синхронные; MCU проверяет кооперативный лимит Duration+10 s,
это не аппаратный watchdog для зависания внутри библиотеки. Время подготовки
не входит в интервал нагрузки. Закрытие COM и сохранение логов добавляют
небольшое время к этим программным пределам.

## Файл и критерии PASS/FAIL

При старте `O_CREAT|O_EXCL` отказывает, если `T09CHECK.BIN` уже существует.
Только после успешного эксклюзивного создания run получает право менять этот
файл. Каждый пакет переписывает собственный файл с усечением до нуля, максимум
128 B. Все остальные имена остаются вне теста. При успешном STOP файл удаляется
и проверяется отсутствие имени; при отказе он может остаться для диагностики.
Сначала сохраните его на PC и убедитесь в происхождении, затем освобождайте
имя для нового прогона. Карта не форматируется автоматически. Потеря питания
или неисправный носитель могут нарушить файловую систему.

PASS требует одновременно:

- Измеренная сборка: Flash <=29 000/32 256 B, static SRAM <=1536/2048 B,
  AVR 1.8.6/1.8.8, SD 1.3.0, Ethernet 2.0.2; hash исходников совпадает с build.
- Один корректный BOOT/READY/START/NET/RESULT с соответствующим токеном;
  реальный W5100, DHCP IPv4, SD/SDHC, ненулевые блоки, FAT16/FAT32.
- Полный интервал нагрузки; минимум max(24, 2×DurationSeconds) точных UDP/SD
  транзакций. Для 60 s это **не менее 120**. Все четыре размера в порядке,
  номера последовательные, каждый ответ без TIMEOUT/потерь/искажения.
- rx=verified=tx=число host-ответов; суммарные bytes и итоговый CRC16 совпадают
  с независимыми host вычислениями. Ни одной ошибки платы или DHCP/IP-смены.
- Как минимум два UART STAT; sampled min_free >=512 B, падение свободной SRAM
  от начальной к конечной <=64 B. Это выборочные точки, не максимум стека
  внутри библиотек или ISR.
- Файл удалён (cleanup=1); оба заключительных RTR равны исходному; RESULT PASS.

Отсутствующие записи, несовпадающие счётчики, повторный BOOT, повреждение
данных, отказ sync/close/read/write, отсутствие DHCP, занятой filename,
неполный интервал и нарушение RAM gate приводят к FAIL.
Результат относится к этому ограниченному UDP/SD прогону на этом экземпляре.
Он не устанавливает полный объём/ресурс карты, TCP/HTTP интеграцию, устойчивость
после отключения сети или сохранность при физическом power cycle во время записи.

## Логи и проверка разработки

`runs/<date>-TEST09-<token>/` содержит `serial.log` с UTC для UART строк,
`probes.csv` (sequence/bytes/CRC/result/RTT), `summary.json` с критериями,
исходными hash и измеренным build. Hardware статус хранится отдельно от BUILD_ONLY.

```powershell
& "$test9\tests\Test-HostGates.ps1"
```

Разработческий `tests/test_firmware_model.py` требует Python 3 и g++:
он компилирует настоящие функции RUN/STOP и SD roundtrip против объявленных
mock-объектов и проверяет отказ при short write, CRC corruption, sync/close.
Это не эмуляция физической SD и не аппаратный PASS.

[Разработческая проверка](VALIDATION_2026-10-10.md).
Исходники зависимостей: [Ethernet 2.0.2](https://github.com/arduino-libraries/Ethernet/tree/2.0.2),
[SD 1.3.0](https://github.com/arduino-libraries/SD/tree/1.3.0).

## Цветной вывод и текущая диагностика

Решения BUILD/RESULT и строки status=PASS выводятся зелёным; FAIL и причины —
красным. Цвет применяется только через Write-Host, поэтому UART/CSV/JSON остаются
без управляющих цветовых кодов. После изменения runner требуется Build-Test09: его
hash входит в measured build gate. Прошивка, протокол и критерии не изменены.

По указанию оператора следующий шаг — отдельная проверка карты прежним TEST-08
v0.2 (запись/CRC/remount), прежде чем продолжать TEST-09. Она требует загрузки
TEST-08; отсутствие сетевой нагрузки в этом прогоне не является TEST-09 PASS.
Ранее предложенный холодный повтор TEST-09 пока не выполнен и остаётся отдельной
возможной диагностикой после проверки карты.
