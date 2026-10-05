#pragma once

#include <Arduino.h>

class HY_M302 {
public:
  struct DhtReading {
    float temperatureC;
    float humidity;
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

  bool button1Pressed() const;
  bool button2Pressed() const;

  void led1(bool on);
  void led2(bool on);

  void setRgbRaw(uint8_t ch1, uint8_t ch2, uint8_t ch3);
  void rgbOff();

  void buzzerTone(unsigned int frequency, unsigned long durationMs = 0);
  void buzzerOff();

  int readPotRaw() const;
  int readLightRaw() const;
  int readLm35Raw() const;
  int readAnalog3Raw() const;

  float readLm35C(float aref = 5.0f) const;

  DhtReading readDht11();

  uint8_t irPin() const { return _pins.ir; }
  const PinMap& pins() const { return _pins; }

private:
  PinMap _pins;

  static bool readDhtBit(uint8_t pin, uint8_t& bit);
};
