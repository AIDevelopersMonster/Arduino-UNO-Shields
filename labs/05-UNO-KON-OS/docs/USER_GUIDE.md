# KON-OS 0.1 — руководство пользователя

## 1. Что такое KON-OS

KON-OS 0.1 — небольшая системная среда для Arduino UNO + MAR2406 microSD.

Пользователь работает с системой через USB Serial terminal.

Версия 0.1 умеет:

- показывать сведения о системе;
- показывать свободную SRAM;
- показывать задачи ядра;
- показывать uptime;
- монтировать microSD;
- просматривать каталоги;
- читать текстовые файлы;
- создавать и перезаписывать текстовый файл;
- дописывать строку;
- создавать/удалять каталог;
- удалять файл;
- перезагружать UNO.

## 2. Запуск

Подключить Arduino UNO к ПК и открыть monitor:

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

После старта:

```text
KON-OS 0.1
Arduino UNO / ATmega328P / 16 MHz
32 KB FLASH / 2 KB SRAM
Cooperative kernel + Serial shell + microSD

BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: ...
Type HELP
A:/>
```

## 3. Приглашение

```text
A:/>
```

означает, что shell готов принять команду.

Команда завершается Enter.

## 4. Регистр

Имя команды нечувствительно к регистру:

```text
DIR /
dir /
Dir /
```

равнозначны.

Имена файлов и текстовые аргументы следует вводить в нужном пользователю
регистре.

## 5. Быстрый старт

Проверить систему:

```text
INFO
MEM
PS
UPTIME
```

Посмотреть карту:

```text
DIR /
```

Создать файл:

```text
WRITE /HELLO.TXT HELLO KON-OS
```

Прочитать:

```text
TYPE /HELLO.TXT
```

Дополнить:

```text
APPEND /HELLO.TXT SECOND LINE
TYPE /HELLO.TXT
```

Удалить:

```text
DEL /HELLO.TXT
```

## 6. Работа с каталогами

Создать:

```text
MKDIR /DATA
```

Показать корень:

```text
DIR /
```

Удалить пустой каталог:

```text
RMDIR /DATA
```

RMDIR предназначен для пустого каталога.

## 7. HELP

```text
HELP
```

выводит текущий список команд.

## 8. MEM

```text
MEM
```

пример:

```text
FREE RAM: 834 B
```

Это приблизительный текущий запас SRAM между heap и stack.

## 9. PS

```text
PS
```

пример:

```text
ID  TASK      PERIOD  RUNS
0   SERIAL    1 ms    102636
1   CLOCK     100 ms  1026
```

Если счётчики RUNS растут, cooperative scheduler работает.

## 10. TYPE / CAT

```text
TYPE /README.TXT
```

или alias:

```text
CAT /README.TXT
```

Команда предназначена для текстовых файлов. Бинарный файл будет выведен как
сырые байты и может испортить отображение terminal.

## 11. WRITE

```text
WRITE <file> <text>
```

Пример:

```text
WRITE /NOTE.TXT FIRST LINE
```

Если файл существует, команда сначала удаляет его и создаёт заново.

KON-OS добавляет newline после текста.

Поэтому:

```text
OK 10 B
```

означает число байтов текста, а физический размер файла может быть на один
байт больше из-за завершающего newline.

## 12. APPEND

```text
APPEND <file> <text>
```

добавляет строку в конец файла и также добавляет newline.

## 13. Ошибки

Частые ответы:

```text
ERR SD_NOT_READY
ERR PATH
ERR TEXT
ERR OPEN
ERR NOT_DIR
ERR IS_DIR
ERR CREATE
ERR DELETE
ERR MKDIR
ERR RMDIR
ERR UNKNOWN COMMAND
```

Если SD не готова:

```text
MOUNT
```

и затем повторить команду.

## 14. CLS

```text
CLS
```

посылает ANSI-команды очистки terminal. Результат зависит от поддержки ANSI
конкретным terminal.

## 15. REBOOT

```text
REBOOT
```

перезапускает ATmega328P через watchdog.

## 16. Практические ограничения

- строка команды — не более 87 символов;
- WRITE/APPEND записывают одну строку за команду;
- не использовать TYPE для больших бинарных файлов;
- текущая версия не имеет COPY, RENAME, RUN и внешних приложений;
- TFT/Touch в версии 0.1 не используются;
- лучше безопасно завершать запись перед отключением питания.

