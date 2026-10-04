/*
  LAB-04 / TEST-01
  Arduino UNO + MAR2406 2.4" TFT Touch Shield
  FreeRTOS scheduler + TFT + Touch monitor

  Architecture:
    - HMI task owns BOTH TFT and Touch because they share shield pins.
    - Worker task runs independently, toggles LED_BUILTIN and sends a heartbeat
      through a FreeRTOS queue.
    - loop() is intentionally empty. The scheduler owns execution after setup().

  Verified hardware from LAB-03:
    LCD: ILI9341, ROT1 / 320x240
    Touch: XP=D6 XM=A2 YP=A1 YM=D7
    Calibration: LEFT=153 RIGHT=930 TOP=962 BOTTOM=168

  Required Arduino library:
    FreeRTOS (Arduino_FreeRTOS_Library for AVR)
*/

#include <Arduino_FreeRTOS.h>
#include <queue.h>

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define YELLOW  0xFFE0
#define GREY    0x8410

#define MINPRESSURE 40
#define MAXPRESSURE 2000

const int XP = 6;
const int XM = A2;
const int YP = A1;
const int YM = 7;

const int TS_LEFT = 153;
const int TS_RT   = 930;
const int TS_TOP  = 962;
const int TS_BOT  = 168;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

QueueHandle_t workerQueue = NULL;

static void TaskHMI(void *pvParameters);
static void TaskWorker(void *pvParameters);

static void restoreSharedPins() {
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(YM, OUTPUT);
}

static bool readTouch(int16_t &x, int16_t &y) {
  TSPoint p = ts.getPoint();
  restoreSharedPins();

  if (p.z <= MINPRESSURE || p.z >= MAXPRESSURE) return false;

  long px = map(p.x, TS_LEFT, TS_RT, 0, 239);
  long py = map(p.y, TS_TOP, TS_BOT, 0, 319);

  px = constrain(px, 0, 239);
  py = constrain(py, 0, 319);

  x = constrain((int16_t)py, 0, 319);
  y = constrain((int16_t)(239 - px), 0, 239);
  return true;
}

static void drawStaticScreen() {
  tft.fillScreen(BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.print(F("FREERTOS / UNO"));

  tft.setTextSize(1);
  tft.setTextColor(CYAN);
  tft.setCursor(214, 11);
  tft.print(F("LAB-04"));

  tft.drawFastHLine(0, 30, 320, GREY);

  tft.setTextColor(WHITE);
  tft.setCursor(12, 46);
  tft.print(F("SCHEDULER"));

  tft.setCursor(12, 72);
  tft.print(F("HMI TASK"));

  tft.setCursor(12, 98);
  tft.print(F("WORKER"));

  tft.setCursor(12, 124);
  tft.print(F("RTOS TICK"));

  tft.setCursor(12, 150);
  tft.print(F("TOUCH"));

  tft.setCursor(12, 176);
  tft.print(F("COORD"));

  tft.drawRect(8, 202, 304, 28, GREY);
  tft.setCursor(18, 212);
  tft.setTextColor(GREY);
  tft.print(F("TFT + TOUCH owned by HMI task"));
}

static void paintValue(int16_t y, uint16_t color, const __FlashStringHelper *label) {
  tft.fillRect(108, y - 3, 202, 18, BLACK);
  tft.setTextColor(color);
  tft.setCursor(108, y);
  tft.print(label);
}

static void showCreateError() {
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.setCursor(38, 85);
  tft.print(F("FREERTOS ERROR"));

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(57, 122);
  tft.print(F("TASK / QUEUE CREATE FAILED"));
}

void setup() {
  Serial.begin(115200);

  tft.reset();
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);

  Serial.println();
  Serial.println(F("LAB-04 TEST-01 FREERTOS"));
  Serial.print(F("LCD ID=0x"));
  Serial.println(id, HEX);

  workerQueue = xQueueCreate(1, sizeof(uint16_t));

  BaseType_t hmiOK = xTaskCreate(
    TaskHMI,
    "HMI",
    280,
    NULL,
    2,
    NULL
  );

  BaseType_t workerOK = xTaskCreate(
    TaskWorker,
    "WORK",
    128,
    NULL,
    1,
    NULL
  );

  if (workerQueue == NULL || hmiOK != pdPASS || workerOK != pdPASS) {
    showCreateError();
    Serial.println(F("ERROR: FreeRTOS object creation failed"));
    for (;;) {}
  }

  Serial.println(F("TASKS CREATED"));
  Serial.println(F("Scheduler starts automatically after setup()"));
}

void loop() {
  // Intentionally empty.
  // Arduino_FreeRTOS starts the scheduler after setup().
}

static void TaskWorker(void *pvParameters) {
  (void)pvParameters;

  pinMode(LED_BUILTIN, OUTPUT);

  uint16_t beat = 0;
  bool led = false;

  for (;;) {
    led = !led;
    digitalWrite(LED_BUILTIN, led ? HIGH : LOW);

    ++beat;
    xQueueOverwrite(workerQueue, &beat);

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
}

static void TaskHMI(void *pvParameters) {
  (void)pvParameters;

  drawStaticScreen();

  paintValue(46, GREEN, F("RUNNING"));
  paintValue(72, CYAN, F("PRIORITY 2"));
  paintValue(98, YELLOW, F("PRIORITY 1"));

  uint16_t workerBeat = 0;
  uint16_t lastBeat = 0xFFFF;
  TickType_t lastTickShown = 0;

  bool lastTouch = false;
  int16_t lastX = -1;
  int16_t lastY = -1;

  for (;;) {
    uint16_t latest;
    if (xQueueReceive(workerQueue, &latest, 0) == pdPASS) {
      workerBeat = latest;
    }

    if (workerBeat != lastBeat) {
      tft.fillRect(184, 95, 126, 18, BLACK);
      tft.setTextColor(YELLOW);
      tft.setCursor(184, 98);
      tft.print(F("BEAT "));
      tft.print(workerBeat);
      lastBeat = workerBeat;
    }

    TickType_t tick = xTaskGetTickCount();
    if ((TickType_t)(tick - lastTickShown) >= (TickType_t)(250 / portTICK_PERIOD_MS)) {
      tft.fillRect(108, 121, 202, 18, BLACK);
      tft.setTextColor(WHITE);
      tft.setCursor(108, 124);
      tft.print((unsigned long)tick);
      lastTickShown = tick;
    }

    int16_t x, y;
    bool pressed = readTouch(x, y);

    if (pressed != lastTouch) {
      paintValue(150, pressed ? GREEN : GREY, pressed ? F("DOWN") : F("UP"));
      lastTouch = pressed;
    }

    if (pressed && (x != lastX || y != lastY)) {
      tft.fillRect(108, 173, 202, 18, BLACK);
      tft.setTextColor(CYAN);
      tft.setCursor(108, 176);
      tft.print(F("X="));
      tft.print(x);
      tft.print(F(" Y="));
      tft.print(y);

      lastX = x;
      lastY = y;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
