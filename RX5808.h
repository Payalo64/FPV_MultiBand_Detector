// RX5808.h
#ifndef RX5808_H
#define RX5808_H

#include <SPI.h>
#include "config.h"

class RX5808 {
public:
  void begin() {
    pinMode(RX5808_CS, OUTPUT);
    digitalWrite(RX5808_CS, HIGH);
    SPI.begin();
  }

  void setChannel(uint8_t ch) {  // 0..47
    uint16_t freq = pgm_read_word(&band_58g_freq[ch]);
    uint16_t val = (freq - 479) / 2;
    uint8_t synth_b = val / 64;
    uint8_t synth_a = val % 64;

    writeReg(0x01, synth_b << 2);
    writeReg(0x00, (synth_a << 1) | 1);
    delayMicroseconds(2000);
  }

  int getRSSI() {
    return analogRead(RX5808_RSSI);  // 0..4095 → ~ -95..-30 dBm
  }

private:
  void writeReg(uint8_t reg, uint8_t data) {
    digitalWrite(RX5808_CS, LOW);
    SPI.transfer(reg << 1);
    SPI.transfer(data);
    digitalWrite(RX5808_CS, HIGH);
  }
};

extern RX5808 rx58;

#endif