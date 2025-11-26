/*
 * FT3500.h - Драйвер для 3.3 GHz (виправлення: повний SPI)
 */

#ifndef FT3500_H
#define FT3500_H

#include <SPI.h>

#define FT3500_CS FT_CS  // З .ino

void setChannelFT3500(int ch) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(FT3500_CS, LOW);
  
  // Команда: reg 0x00 для каналу (8-bit, group + ch)
  byte group = ch / 8;
  byte subch = ch % 8;
  SPI.transfer(0x00);  // Reg
  SPI.transfer((group << 3) | subch);  // Data
  
  digitalWrite(FT3500_CS, HIGH);
  SPI.endTransaction();
}

int getRSSI_FT3500() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(FT3500_CS, LOW);
  
  SPI.transfer(0x1A);  // RSSI reg
  int rssi = SPI.transfer(0x00);  // Read
  
  digitalWrite(FT3500_CS, HIGH);
  SPI.endTransaction();
  
  return (rssi > 0xFF) ? -100 : (int)rssi - 128;  // Scale to dBm (приклад; калібруйте)
}

#endif