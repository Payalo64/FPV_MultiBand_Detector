// SM1370R.h
#ifndef SM1370R_H
#define SM1370R_H

#include "config.h"

class SM1370R {
public:
  void begin() {
    pinMode(PIN_12G_A, OUTPUT);
    pinMode(PIN_12G_B, OUTPUT);
    pinMode(PIN_12G_C, OUTPUT);
  }

  void setChannel(uint8_t ch) {  // 0..7
    digitalWrite(PIN_12G_A, ch & 1);
    digitalWrite(PIN_12G_B, ch & 2);
    digitalWrite(PIN_12G_C, ch & 4);
  }

  int getRSSI() {
    int raw = analogRead(PIN_RSSI_12G);
    return map(raw, 0, 4095, -100, -25);
  }
};

extern SM1370R sm12;

#endif