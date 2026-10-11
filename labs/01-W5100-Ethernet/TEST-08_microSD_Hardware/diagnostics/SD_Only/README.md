# SD_ONLY v0.1 — отдельная диагностика SD на UNO

Подготовлена после фактического USB-SD PASS 16 MiB и двух TEST-08 v0.4
ETH_SPI_BEFORE FAIL. **Аппаратный SD_ONLY PASS, 13/13: B652B736, 10 октября 2026.**
Полный TEST-08 v0.4 и TEST-09 v0.2 не изменены. Этот результат называется
SD_ONLY, а не полный PASS TEST-08.

Тот же UNO/W5100 Shield, Ethernet CS=D10, SD CS=D4, UART 115200. Ethernet
остаётся физически на общей шине, но D10 устанавливается HIGH до OUTPUT и
прошивка не выбирает его, не читает/пишет его регистры и не запускает сеть.
Все последующие SPI-транзакции выполняются библиотекой SD 1.3.0. SD init
250 kHz, SD data 4 MHz, как в TEST-08. Частота 0 в поле eth_spi_hz означает
отсутствие адресованных Ethernet-транзакций, а не отсутствие общих SD-тактов.
Эта конфигурация изолирует программные обращения к W5100; она не отключает
электрическое влияние физически установленного Ethernet-чипа.

Файловые функции перенесены из TEST-08 v0.4: детерминированный образец,
write/sync/close, побайтовое чтение с CRC/EOF, seek, append, сброс cache,
remount и cleanup. Используется отдельное имя SDONLY.BIN с O_EXCL; занятое
имя даёт FAIL без перезаписи. В конце bytes — длина последнего успешно
проверенного readback; planned_bytes=2112 отдельно указывает план.

## Возврат карты и команды PowerShell 7

Выполнить Безопасное извлечение USB-картридера в Windows и отключить его USB.
Вставить именно проверенную карту в обесточенный UNO/Shield, затем подключить
USB UNO к компьютеру. Ethernet-кабель можно оставить прежним. Закрыть монитор
Arduino; во время автоматического прогона действий с картой не выполнять.

```powershell
git pull --ff-only
$sdOnly = ".\labs\01-W5100-Ethernet\TEST-08_microSD_Hardware\diagnostics\SD_Only"
& "$sdOnly\Build-SdOnly.ps1" -UploadPort COM4
& "$sdOnly\Test-SdOnly.ps1" -SerialPort COM4 -DurationSeconds 90
```

Пример физической подготовки с пояснениями/Enter:

```powershell
$null = Read-Host "Безопасно извлеки USB-картридер в Windows и отключи его USB. Затем нажми Enter"
$null = Read-Host "Отключи всё питание UNO. Вставь проверенную microSD в Shield, затем подключи USB UNO. После подключения нажми Enter"
```

Build helper проверяет AVR 1.8.6/1.8.8 и SD 1.3.0, Flash <=29000 B,
static SRAM <=1200 B, сохраняет compile.json, build-summary.json и хеши
исходников/HEX в build/SD_ONLY. Загрузка использует измеренный бинарник.
Runner требует подходящие build/hash/BOOT, самостоятельно отправляет RUN с
токеном и пишет serial.log/summary.json в собственный runs. Цвета PASS/FAIL
зелёный/красный. Ровно один BOOT и READY, один совпадающий START/RESULT.

## Критерии

Нужны все 13 CHECK в порядке и каждый PASS:
CARD_INIT, CARD_INFO, FAT_VOLUME, ROOT_OPEN, EXCLUSIVE_CREATE, WRITE_2048,
REOPEN_VERIFY_2048, SEEK_BOUNDARIES, APPEND_64, REOPEN_VERIFY_2112,
REMOUNT_VERIFY, REMOVE_TEST_FILE, RAM.

CARD code/data=0/0; тип SD1/SD2/SDHC и ненулевой размер; FAT16/FAT32;
три точных readback: 2048 B / CRC A535, 2112 B / CRC 2B28, 2112 B / CRC 2B28.
Каждый байт сравнивается на UNO, host вычисляет эталонные CRC независимо.
Финальные checks=13, failures=0, bus_fault=0, bytes=2112, crc16=2B28;
sampled min_free >=512 B, drift <=64 B. Ошибки файлового close учитываются.
Занятый CS защёлкивает bus_fault и останавливает дальнейшие операции.
Полный TEST-08 BOOT, W5100 ETH_RTR record или отсутствующий этап дают FAIL.

90 s — предел ожидания ответа host, а не длительность нагрузки. SD-вызовы
синхронные; у MCU нет аппаратного watchdog. Размер/память и sampled free RAM
не являются проверкой всей поверхности, долговременного хранения или пика
стека. FAIL сохраняется; повторов до получения PASS нет.

Если этот прогон PASS, он подтвердит файловые операции через UNO/SD при
отсутствии программных обращений к W5100. Это поддержит локализацию проблемы
на доступе W5100/переходе между устройствами, но само по себе не докажет,
что виноват конкретный код, карта или электрическая часть общей шины.

## Проверка подготовленного кода — 10 октября 2026

Фактическая локальная сборка Arduino CLI 1.3.1, AVR 1.8.6, SD 1.3.0:
**13 758 B Flash / 1 050 B static SRAM**, compiler stderr пуст.
Использованы только библиотеки SPI 1.0 и SD 1.3.0. AVR 1.8.8 пользователя
должен получить собственные измерения и хеш бинарника при сборке.

Все четыре PowerShell-файла проходят разбор. 36 синтетических случаев
host verdict проходят, включая неполный/чужой BOOT, неверный токен,
отсутствующий этап, неверный CRC, ошибки CARD, bus_fault и RAM.
Это проверка программы принятия решения, не аппаратный PASS.
Независимый расчёт образца Python CRC-HQX подтверждает A535/2B28.

Проверка исходника подтверждает отсутствие Ethernet.h, SPI.transfer,
чтения RTR и установки ETH_CS в LOW; ровно 13 вызовов CHECK.
Исходники полных TEST-08/09 остаются прежними.

SHA-256 диагностического SD_Only.ino:
`7B6702EC1BA40A1107206D5E54246E519B3D97D1F6007FD29485D8834DCE818C`.
SHA-256 локального HEX без bootloader:
`BC6D411DADEFC44B1B87823263DF93DD04EE26F4856CDFBF5DAF4686466143CD`.
Хеши runner/module совпадают с финальным build-summary.json.
**Аппаратный результат SD_ONLY: PASS для прогона B652B736.**


## Фактический прогон B652B736

[Полная консоль](../../evidence/2026-10-10-sd-only-202038-pass-console.txt):
`20261010-202038-SD_ONLY-B652B736`, COM4, один корректный BOOT/READY.
Все 13 CHECK PASS, failures=0, bus_fault=0; SDHC, 7 864 320 блоков, FAT32.
Побайтовые readback: 2048 B / A535, 2112 B / 2B28, remount 2112 B / 2B28.
Свой файл удалён; free=957 B до/после, sampled min_free=938 B, elapsed=443 ms.
Неизменённый Get-SdOnlyVerdict повторно оценил supplied UART как PASS без причин.
Операторские build-summary.json, HEX/source hashes и полный serial.log не
предоставлены; их содержимое не реконструируется. Это ограниченный SD_ONLY
результат; полные TEST-08 v0.4 FAIL остаются FAIL. Причина RTR-сбоев не доказана.

Последующая [W5100_COMPARE](../W5100_Compare/) дала ETH_DRIVER_INIT FAIL
(`886B359A`), до RTR и файловых операций. Там сохранены результат и команды
следующего независимого контроля TEST-07 Baseline без microSD. SD_ONLY PASS
сохраняется; причину общего SPI-отказа эти прогоны пока не устанавливают.
