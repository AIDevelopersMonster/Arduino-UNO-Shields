# KON Relay & Control Lab

> **Repository status: FROZEN / AUXILIARY (2026-10-11).** The bounded four-channel USB/Serial profile is retained as a useful Arduino UNO accessory. The former RS-485 / Modbus / KON-PLC Lite roadmap is **deferred with no active schedule**. Reopen only for a concrete auxiliary task or when moving the control concept to a more appropriate modern platform.

Статус: **STAGE-02 BASIC USB/SERIAL CONTROL BENCH PASS** (2026-10-11). Расширенная силовая/Modbus сертификация не выполнена.
Исторически планировалась траектория **4-relay Arduino UNO → KON_RelayShield → UART/RS-485 → Modbus RTU → KON-PLC Lite**; после заморозки Arduino UNO-направления она сохранена только как справочная архитектура.
Bluetooth UART/BLE и KSC Target — отдельные последующие адаптеры, а не зависимости базовой библиотеки.

## Цель этапа 01

1. Зафиксировать паспорт **семейства** Relay Shield по предоставленной фотографии, не выдавая картинку продавца за измерение экземпляра.
2. Организовать аудит передней/задней стороны, вывода каналов, полярности, 5 В питания, NO/NC/COM и поведения при RESET.
3. Утвердить ТЗ независимой Arduino-библиотеки и требования к безопасному состоянию после `begin()`.
4. Подготовить измерительные процедуры и стартовую прошивку, **не включающую реле автоматически**.
5. Проверить будущий pin-budget и разделение Modbus RTU / Bluetooth / KSC.

## Содержание

- [Hardware audit](hardware/README.md) — свидетельства, неизвестные параметры, допуски и stop-условия.
- [Pin/resource matrix](hardware/pin_matrix.csv) — предполагаемые занятости UNO (статусы НЕ измерены).
- [Library technical specification](docs/LIBRARY_SPEC_v0.1.md) — минимальная классическая библиотека, вне KonSol.
- [Transport architecture](docs/ARCHITECTURE_v0.1.md) — RS-485 UART 8E1, Modbus, будущий PLC.
- [Test matrix](tests/TEST_MATRIX.md) — пошаговая программа с проверяемыми PASS/FAIL.
- [Test result template](tests/RESULTS_TEMPLATE.md) — форма записи измерений и наблюдений.
- [01_PinProbe](firmware/01_PinProbe/01_PinProbe.ino) — исходный пассивный диагностический скетч.
- [02_SerialControl](firmware/02_SerialControl/02_SerialControl.ino) — **BENCH PASS**: USB/COM 115200, R1–R4 на D4–D7, ACTIVE HIGH.
- [Library placeholder](libraries/KON_RelayShield/README.md) — граница работ следующего этапа.
- [Host placeholder](host/README.md) — историческая заготовка; дальнейшее развитие не запланировано.

## Структурный принцип

`hardware` описывает факты и гипотезы; `libraries` управляет только GPIO; `firmware` объединяет драйвер с протоколами; `host` предоставляет управление и тестовые клиенты. Никакие результаты испытаний на настоящем оборудовании не утверждаются на основании компиляции или чтения документации.

## Первые действия оператора

1. Снять фото **обеих сторон именно своей платы**, отдельно правой области с пайкой и маркировкой клемм.
2. Без USB, внешнего питания и нагрузки прозвонить цепи и определить D-пины каналов; записать NO/NC/COM.
3. Подключать UNO только после проверки на замыкания между `5V`/`GND` и силовыми контактами.
4. Загрузить `01_PinProbe` в начальной версии, убедиться, что скетч **не управляет GPIO**.
5. После проверки полярности и **без любых нагрузок на клеммах** можно вручную включить `ENABLE_STIMULUS=1`, пересобрать и с осторожностью проверить по одному выводу.

**Ограничение:** никакие испытания на 220/230 В, сетевой нагрузке и силовых цепях в этом этапе не предусмотрены.

## Связь с репозиторием

Это новый независимый проект. `projects/KSC-KonSol-Commander` и `labs/05-UNO-KON-OS` не изменяются. Стадия 01 не означает выпуск готового релейного контроллера и не означает Modbus PASS.


## Bench result 2026-10-11

Фактически подтверждено на Arduino UNO с данным 4-relay shield: **R1=D4, R2=D5, R3=D6, R4=D7**, логика входов **ACTIVE HIGH**, управление всеми четырьмя каналами по USB/COM 115200 работает. Базовый Serial controller принят как рабочий профиль. Это не является сертификацией силовых контактов, токов, 230 В, RESET-глитчей или Modbus.


## Рабочие профили

- [02_SerialControl](firmware/02_SerialControl/02_SerialControl.ino) — **BENCH PASS**. Arduino UNO + 4-relay shield, R1=D4, R2=D5, R3=D6, R4=D7, ACTIVE HIGH, управление по USB/COM 115200.
- [03_EthernetControl](firmware/03_EthernetControl/) — W5100 Ethernet + 4-relay control. Web UI, HTTP JSON API, Serial CLI, PowerShell CLI и Windows GUI. microSD должна быть физически удалена, поскольку D4 используется как R1.

Таким образом, все варианты управления этим релейным шилдом теперь собраны под `projects/KON-Relay-Control/`, а не разбросаны по отдельным лабораториям.
