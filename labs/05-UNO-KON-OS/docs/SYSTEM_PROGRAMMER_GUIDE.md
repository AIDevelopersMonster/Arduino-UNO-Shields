# KON-OS 0.1 — руководство системного программиста

## 1. Назначение документа

Документ предназначен для разработчика ядра, драйверов и системных сервисов
KON-OS.

Исходный файл TEST-01:

`sketches/01_KONOS_Shell_SD/01_KONOS_Shell_SD.ino`

## 2. Основные правила системного кода

Для ATmega328P действуют жёсткие ограничения:

- не использовать Arduino `String`;
- не использовать `malloc()` / `new` в коде KON-OS;
- постоянные строки выводить через `F("...")`;
- не создавать большие локальные массивы;
- использовать фиксированные глобальные буферы только при необходимости;
- не вводить отдельный stack на каждую задачу;
- кооперативная задача должна быстро возвращать управление;
- D13 при активной SD использовать только как SPI SCK.

## 3. Точка входа

```cpp
void setup() {
  ...
  kernelInit();
  ...
}

void loop() {
  kernelDispatch();
}
```

`loop()` не является прикладным циклом. Это dispatcher ядра.

## 4. Таблица задач

Тип задачи:

```cpp
typedef void (*TaskFunction)();

struct KernelTask {
  TaskFunction fn;
  uint16_t periodMs;
  uint32_t nextRun;
  uint32_t runs;
  bool enabled;
};
```

Текущая таблица:

```cpp
static KernelTask tasks[] = {
  { taskSerial, 1,   0, 0, true },
  { taskClock,  100, 0, 0, true }
};
```

### Добавление системной задачи

1. Написать функцию без параметров и результата:

```cpp
static void taskExample() {
  // Выполнить небольшой квант работы.
  // Не использовать бесконечный цикл.
}
```

2. Добавить запись в `tasks[]`:

```cpp
{ taskExample, 50, 0, 0, true }
```

3. Обновить `PS`, если требуется человекочитаемое имя.

### Требование кооперативности

Недопустимо:

```cpp
static void taskExample() {
  while (true) {
    ...
  }
}
```

Допустимо:

```cpp
static void taskExample() {
  if (!workPending()) return;
  doOneSmallStep();
}
```

Длинная блокирующая функция останавливает все остальные задачи ядра.

## 5. Планирование

`kernelDispatch()` сравнивает `millis()` с `nextRun`.

После запуска:

```cpp
t.nextRun += t.periodMs;
```

используется предыдущий deadline, а не текущее время. Это уменьшает
долговременный дрейф периодических задач.

KON-OS 0.1 не гарантирует hard real-time deadlines.

## 6. Память

Диагностика свободной SRAM реализована через разность текущей вершины стека и
конца heap:

```text
FREE RAM = stack_top - heap_top
```

Физически измерено:

```text
1009 B после boot
834 B в нормальном shell
```

### Политика изменения ядра

После каждого существенного изменения необходимо записывать:

- Flash bytes / percent;
- global SRAM bytes / percent;
- FREE RAM после boot;
- FREE RAM после серии файловых операций.

Рекомендуемый минимальный рабочий reserve для экспериментальной ветки:
не приближаться к нулю и прекращать добавление функций при признаках
нестабильности, reset, повреждения stack/SD или резкого падения FREE RAM.

## 7. Командный parser

Размер входного буфера:

```cpp
const uint8_t CMD_SIZE = 88;
```

Максимальная полезная строка — 87 символов.

Алгоритм:

1. trim leading spaces;
2. имя команды переводится в upper case;
3. первая последовательность пробелов разделяет command и args;
4. args сохраняет исходный регистр;
5. выполняется command handler.

### Добавление системной команды

1. Написать handler:

```cpp
static void cmdExample(char *args) {
  ...
}
```

2. Добавить ветку в `executeCommand()`:

```cpp
} else if (strcmp(line, "EXAMPLE") == 0) {
  cmdExample(args);
```

3. Добавить строку в `cmdHelp()`.

4. Дополнить COMMAND_REFERENCE.md.

## 8. Работа с SD

Инициализация:

```cpp
SD.begin(10)
```

Перед файловой операцией handler должен проверять `sdReady`.

Типовой шаблон:

```cpp
if (!sdReady) {
  Serial.println(F("ERR SD_NOT_READY"));
  return;
}
```

Каждый открытый `File` должен закрываться по всем веткам выхода.

## 9. Системные ошибки

Текущие текстовые ошибки:

```text
ERR SD_NOT_READY
ERR PATH
ERR TEXT
ERR OPEN
ERR NOT_DIR
ERR IS_DIR
ERR REMOVE_OLD
ERR CREATE
ERR DELETE
ERR MKDIR
ERR RMDIR
ERR UNKNOWN COMMAND
```

Новая системная функция должна использовать короткие детерминированные ошибки,
а не длинные диагностические сообщения, чтобы экономить Flash.

## 10. Watchdog reset

Команда REBOOT включает WDT на 15 ms и входит в бесконечный цикл:

```cpp
wdt_enable(WDTO_15MS);
for (;;) {}
```

При boot:

```cpp
MCUSR = 0;
wdt_disable();
```

обязательно отключает watchdog после reset.

## 11. Изменение системной архитектуры

Перед добавлением TFT, Touch, VM или KON-Boot системный программист должен
проверить три бюджета:

```text
Flash
SRAM globals
runtime free RAM / stack reserve
```

Оптимизация должна быть архитектурной: сокращение generic libraries,
статические структуры, маленькие сервисы, одна очередь выполнения.

