#pragma once

#include <Arduino.h>

class HY_M302 {
public:
  struct DhtReading {
    float temperatureC;
    float humidity;
    bool ok;
  };

  struct IrNecFrame {
    uint16_t address;
    uint8_t command;
    uint32_t raw;
    bool repeat;
    bool ok;
  };

  struct PinMap {
    uint8_t sw1 = 2;
    uint8_t sw2 = 3;
    uint8_t dht = 4;
    uint8_t buzzer = 5;
    uint8_t ir = 6;
    uint8_t gpio7 = 7;
    uint8_t gpio8 = 8;
    uint8_t rgb1 = 9;
    uint8_t rgb2 = 10;
    uint8_t rgb3 = 11;
    uint8_t led2 = 12;
    uint8_t led1 = 13;
    uint8_t pot = A0;
    uint8_t light = A1;
    uint8_t lm35 = A2;
    uint8_t analog3 = A3;
  };

  HY_M302();
  explicit HY_M302(const PinMap& pins);

  void begin();
  // Cooperative service hook. Call from loop/task code when async drivers are used.
  void service();

  bool button1Pressed() const;
  bool button2Pressed() const;

  void led1(bool on);   // D13, blue discrete LED on tested sample
  void led2(bool on);   // D12, red discrete LED on tested sample
  void ledBlue(bool on);
  void ledRed(bool on);

  // Bench-certified on our HY-M302 sample:
  // D9=RED, D10=GREEN, D11=BLUE, direct PWM polarity (0=off, 255=full).
  void setRGB(uint8_t red, uint8_t green, uint8_t blue);
  void setRgbRaw(uint8_t ch1, uint8_t ch2, uint8_t ch3);
  void rgbOff();

  // Bench-certified on our HY-M302 sample: active/self-oscillating buzzer on D5.
  void buzzerOn();
  void buzzerOff();
  // Optional PWM/tone drive retained for experiments and clone compatibility.
  void buzzerTone(unsigned int frequency, unsigned long durationMs = 0);

  int readPotRaw() const;
  int readLightRaw() const;
  int readLm35Raw() const;
  int readAnalog3Raw() const;
  float readLm35C(float aref = 5.0f) const;

  DhtReading readDht11();

  // Legacy blocking NEC decoder retained for comparison/testing.
  bool readIrNec(IrNecFrame& frame, unsigned long startTimeoutUs = 15000UL);

  // Non-blocking NEC receiver for the standard HY-M302 D6 wiring on UNO.
  // ISR captures edges only; call serviceIrNec() frequently from loop/task code.
  bool beginIrNecAsync();
  void endIrNecAsync();
  void serviceIrNec();
  bool irNecAvailable() const;
  bool readIrNecAsync(IrNecFrame& frame);
  uint16_t irNecDroppedEdges() const;
  uint16_t irNecDroppedFrames() const;
  void resetIrNecStats();

  void gpio7Mode(uint8_t mode);
  void gpio8Mode(uint8_t mode);
  int gpio7Read() const;
  int gpio8Read() const;
  void gpio7Write(uint8_t value);
  void gpio8Write(uint8_t value);

  uint8_t irPin() const { return _pins.ir; }
  const PinMap& pins() const { return _pins; }

private:
  PinMap _pins;

  static uint32_t expectPulse(volatile uint8_t* inputReg, uint8_t bitMask,
                              uint8_t level, uint32_t maxLoops);
  static unsigned long measureCurrentPulseUs(volatile uint8_t* inputReg,
                                             uint8_t bitMask,
                                             uint8_t level,
                                             unsigned long timeoutUs);
  static bool inRange(unsigned long value, unsigned long minUs, unsigned long maxUs);

  enum IrAsyncState : uint8_t {
    IR_WAIT_LEADER_LOW = 0,
    IR_WAIT_LEADER_HIGH,
    IR_WAIT_REPEAT_LOW,
    IR_WAIT_BIT_LOW,
    IR_WAIT_BIT_HIGH
  };

  void processIrAsyncPulse(uint8_t level, uint16_t durationUs);
  void queueIrAsyncFrame(const IrNecFrame& frame);
  void resetIrAsyncDecoder();

  bool _irAsyncEnabled = false;
  uint8_t _irAsyncState = IR_WAIT_LEADER_LOW;
  uint8_t _irAsyncBitIndex = 0;
  uint32_t _irAsyncRaw = 0;

  static const uint8_t IR_ASYNC_FRAME_QUEUE_SIZE = 4;
  IrNecFrame _irAsyncFrames[IR_ASYNC_FRAME_QUEUE_SIZE];
  uint8_t _irAsyncFrameHead = 0;
  uint8_t _irAsyncFrameTail = 0;
  uint16_t _irAsyncDroppedFrames = 0;

  IrNecFrame _irAsyncLastFull = {0, 0, 0, false, false};
};
