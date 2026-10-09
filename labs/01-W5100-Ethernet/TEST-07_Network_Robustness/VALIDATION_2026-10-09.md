# TEST-07 v0.1 — проверка разработки, 2026-10-09

**BUILD VERIFIED / HOST CHECKS PASSED / HARDWARE PENDING.**
Физическая UNO/W5100 не подключалась к среде разработки; аппаратный PASS
TEST-07 не присвоен. Этот отчёт не переносит результаты TEST-05/06 на TEST-07.

## Фактически выполнено

| Проверка | Результат |
| --- | --- |
| Arduino CLI 1.3.1, `arduino:avr:uno`, AVR core 1.8.6, Ethernet 2.0.2, Linux x86_64 | Компиляция успешна |
| Flash / предел платы | **20 394 / 32 256 B** (63.2%); budget <=29 000 B выполнен |
| Статическая SRAM / предел платы | **879 / 2048 B** (42.9%); budget <=1200 B выполнен |
| Статический остаток SRAM | **1169 B**; реальный stack peak не измерялся |
| `Build-Test07.ps1` через PowerShell 7.5.2 | Собрал HEX, проверил выбранные версии и memory gate, записал SHA256 |
| PowerShell parser, все `.ps1/.psm1` комплекта | Ошибок синтаксиса нет |
| `tests/Invoke-HostSelfTest.ps1`, PowerShell 7.5.2 + Python 3 | Все проверки прошли: положительные/отрицательные synthetic verdicts и реальный loopback UDP/TCP |
| UDP loopback | Точное 128 B эхо принято; повреждение, неверная длина/порт источника и тайм-аут отвергнуты |
| TCP loopback | Точное 64 B эхо + EOF принято; неверный ответ отвергнут; RST незавершённой строки сформирован |
| Host runner, 30 s Baseline, программный UART-адаптер + настоящий loopback транспорт | Healthy-фаза, 264 UDP, 6 TCP, четыре длины; ожидаемый положительный **mock** вердикт |
| Host runner, 60 s Cable, программный fault/UART + loopback | HEALTHY → OUTAGE → RECOVERY → POST; 14 failed outage probes, 231 post UDP и 5 post TCP; ожидаемый положительный **mock** вердикт |
| Host runner, повреждение ответов после healthy warmup | 192 corrupted probes; ожидаемый FAIL, а не ложный PASS |
| Certification aggregator на synthetic JSON | Полный корректный fixture принят; отсутствие natural renewal отвергнуто |
| `git diff --check` | Без ошибок whitespace |

Для проверки host state machine использовалась временная копия runner, в которой
только COM-адаптер заменён читаемым файлом UART-событий; транспортные функции
выполняли настоящие локальные UDP/TCP-запросы. Это проверка логики скрипта,
не электроники, DHCP-сервера или Windows COM/DTR. Модельные миллисекунды recovery
не являются результатом измерения W5100 и не включены в аппаратный отчёт.

При `--warnings all` остались только четыре предупреждения о неиспользуемом
`tag` в `cores/arduino/new.cpp` AVR core 1.8.6. Предупреждений в исходнике TEST-07
в финальной сборке нет. Первый вариант с `int`/`uint16_t` сравнением был исправлен
до финальной сборки.

SHA256 проверенного firmware source:

`97CE09C4649E29225094032EEAA8AF7DEE88F4CADC7F74A316A56262DF8879A2`

SHA256 полученного в этой сборке HEX:

`F33EAECDA51257B13E7F95D6E4B257E2F8B1162026551A2E1F66B72C1022B0E3`

Размеры и hash своей Windows-сборки следует сохранять через `Build-Test07.ps1`;
они могут отличаться при другом toolchain или параметрах сборки.

## Что остаётся проверить физически

Все семь сценариев из README: Baseline, Cable, StartupDhcp, DhcpRenew,
DhcpOutage, Soak, TcpAbort. Пока не измерены время восстановления W5100,
реальная DHCP-аренда/renewal, потери/ошибки после fault, выборочный runtime SRAM
и отсутствие reset при работе платы. Windows COM/DTR и загрузка прошивки здесь
не выполнялись. Для FULL PASS нужны фактические журналы и заполненный отчёт
по конкретному образцу, а не результаты mock/self-test.
