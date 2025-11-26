// FT3500.h
#ifndef FT3500_H
#define FT3500_H

#include <SPI.h>
#include "config.h"

class FT3500 {
public:
  void begin() {
    pinMode(FT_CS, OUTPUT);
    digitalWrite(FT_CS, HIGH);
    SPI.begin();
  }

  void setChannel(uint8_t ch) {  // 0..63
    uint8_t group = ch / 8;
    uint8_t sub   = ch % 8;

    digitalWrite(FT_CS, LOW);
    SPI.transfer(0x00);
    SPI.transfer((group << 3) | sub);
    digitalWrite(FT_CS, HIGH);
    delayMicroseconds(3000);
  }

  int getRSSI() {
    int raw = analogRead(FT_RSSI_PIN);
    // Калібровка (приблизно)
    return map(raw, 0, 4095, -100, -30);
  }
};

extern FT3500 ft33;

#endif