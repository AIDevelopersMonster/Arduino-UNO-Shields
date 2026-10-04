# KON-OS 0.1 — справочник синтаксиса команд

## Общая форма

```text
COMMAND [arguments]
```

Имя команды нечувствительно к регистру.

Максимальная длина всей введённой строки: **87 символов**.

Prompt:

```text
A:/>
```

## HELP

```text
HELP
```

Показывает список команд.

Аргументы: нет.

## INFO

```text
INFO
```

Показывает:

- версию KON-OS;
- CPU;
- Flash;
- SRAM;
- тип scheduler;
- число задач;
- состояние SD;
- свободную RAM.

## MEM

```text
MEM
```

Показывает текущую оценку свободной SRAM.

## UPTIME

```text
UPTIME
```

Вывод:

```text
UPTIME: <milliseconds> ms
KERNEL TICKS: <ticks>
```

## PS

```text
PS
```

Выводит таблицу кооперативных задач:

```text
ID  TASK      PERIOD  RUNS
0   SERIAL    1 ms    ...
1   CLOCK     100 ms  ...
```

## MOUNT

```text
MOUNT
```

Повторно инициализирует microSD.

Ответ:

```text
MOUNT BEGIN
MOUNT PASS
```

или:

```text
MOUNT FAIL
```

## DIR

```text
DIR [path]
```

Если path не указан, используется `/`.

Примеры:

```text
DIR
DIR /
DIR /DATA
```

Формат строки:

```text
F <size> <name>
D <size> <name>
```

где:

- F — file;
- D — directory.

Для каталогов значение size из SD library не следует трактовать как реальный
размер содержимого каталога.

Alias:

```text
LS
```

## LS

Полный alias команды DIR.

```text
LS [path]
```

## TYPE

```text
TYPE <file>
```

Печатает содержимое файла между разделителями:

```text
-----
...
-----
```

Пример:

```text
TYPE /XOLOG.TXT
```

Alias: `CAT`.

## CAT

Полный alias команды TYPE.

```text
CAT <file>
```

## WRITE

```text
WRITE <file> <text>
```

Перезаписывает файл одной строкой.

Пример:

```text
WRITE /KONTEST.TXT HELLO FROM KON-OS
```

Если файл уже существует, он удаляется и создаётся заново.

После текста записывается newline.

Ответ:

```text
OK <text-bytes> B
```

Счётчик относится к тексту и не включает добавленный newline.

## APPEND

```text
APPEND <file> <text>
```

Добавляет строку в конец файла.

Пример:

```text
APPEND /KONTEST.TXT SECOND LINE
```

После текста добавляется newline.

## DEL

```text
DEL <file>
```

Удаляет файл.

Пример:

```text
DEL /KONTEST.TXT
```

Alias: `RM`.

## RM

Полный alias DEL.

```text
RM <file>
```

## MKDIR

```text
MKDIR <path>
```

Создаёт каталог.

Пример:

```text
MKDIR /KON
```

## RMDIR

```text
RMDIR <path>
```

Удаляет пустой каталог.

Пример:

```text
RMDIR /KON
```

## CLS

```text
CLS
```

Отправляет в Serial terminal:

```text
ESC [2J
ESC [H
```

то есть ANSI clear screen + cursor home.

## REBOOT

```text
REBOOT
```

Печатает:

```text
REBOOT
```

затем вызывает watchdog reset.

## Таблица команд

| Команда | Alias | Синтаксис | Назначение |
| --- | --- | --- | --- |
| HELP | — | HELP | список команд |
| INFO | — | INFO | сведения о системе |
| MEM | — | MEM | свободная SRAM |
| UPTIME | — | UPTIME | uptime и kernel ticks |
| PS | — | PS | таблица задач |
| MOUNT | — | MOUNT | повторное монтирование SD |
| DIR | LS | DIR [path] | каталог |
| TYPE | CAT | TYPE <file> | печать текстового файла |
| WRITE | — | WRITE <file> <text> | заменить файл строкой |
| APPEND | — | APPEND <file> <text> | добавить строку |
| DEL | RM | DEL <file> | удалить файл |
| MKDIR | — | MKDIR <path> | создать каталог |
| RMDIR | — | RMDIR <path> | удалить пустой каталог |
| CLS | — | CLS | очистить ANSI terminal |
| REBOOT | — | REBOOT | watchdog reset |

## Ошибки

| Ответ | Значение |
| --- | --- |
| ERR SD_NOT_READY | SD не смонтирована |
| ERR PATH | не указан путь |
| ERR TEXT | не указан текст WRITE/APPEND |
| ERR OPEN | файл/каталог не открыт |
| ERR NOT_DIR | указанный объект не каталог |
| ERR IS_DIR | TYPE получил каталог |
| ERR REMOVE_OLD | старый файл не удалось удалить перед WRITE |
| ERR CREATE | файл не удалось открыть для записи |
| ERR DELETE | файл не удалён |
| ERR MKDIR | каталог не создан |
| ERR RMDIR | каталог не удалён |
| ERR UNKNOWN COMMAND | неизвестная команда |

