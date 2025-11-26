// FPV_MultiBand_Detector_v3.ino
#include <WiFi.h>
#include <U8g2lib.h>
#include "config.h"
#include "Bands.h"
#include "RX5808.h"
#include "FT3500.h"
#include "SM1370R.h"

// Об'єкти
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
RX5808 rx58;
FT3500 ft33;
SM1370R sm12;

// Глобальні змінні
enum Band { B12G, B33G, B58G };
Band currentBand = B58G;
uint8_t currentCh = 0;
int currentRSSI = -100;
int bestRSSI = -100;
uint8_t bestCh = 0;
uint8_t mode = 0; // 0=MONITOR, 1=SCAN, 2=DETECTOR

unsigned long lastScan = 0;
unsigned long lastUI = 0;
unsigned long alarmEnd = 0;
bool scanning = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== FPV MultiBand Detector v3.0 FINAL ===");

  WiFi.mode(WIFI_OFF);
  btStop();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(10, 30, "FPV DETECTOR v3");
  u8g2.sendBuffer();
  delay(1500);

  rx58.begin();
  ft33.begin();
  sm12.begin();

  esp_task_wdt_init(15, true);
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset();

  handleButtons();
  runScanner();
  updateDisplay();
}

void handleButtons() {
  static unsigned long lastPress = 0;
  if (millis() - lastPress < 250) return;

  if (!digitalRead(BTN_MODE)) {
    mode = (mode + 1) % 3;
    scanning = false;
    lastPress = millis();
  }
  if (!digitalRead(BTN_UP)) {
    currentCh = (currentCh + 1) % getChannelCount();
    lastPress = millis();
  }
  if (!digitalRead(BTN_DOWN)) {
    currentCh = (currentCh == 0) ? getChannelCount() - 1 : currentCh - 1;
    lastPress = millis();
  }
}

void runScanner() {
  if (mode == 0) { // MONITOR
    setChannel(currentBand, currentCh);
    currentRSSI = readRSSI(currentBand);
    scanning = false;
  }

  if (mode == 1 || mode == 2) { // SCAN або DETECTOR
    if (millis() - lastScan >= SCAN_DWELL_TIME) {
      setChannel(currentBand, currentCh);
      currentRSSI = readRSSI(currentBand);

      if (currentRSSI > bestRSSI) {
        bestRSSI = currentRSSI;
        bestCh = currentCh;
      }

      currentCh++;
      if (currentCh >= getChannelCount()) {
        currentCh = 0;
        if (mode == 2) {
          currentBand = (Band)((currentBand + 1) % 3);
        }
        if (mode == 1) {
          currentCh = bestCh;
          scanning = false;
        }
      }
      lastScan = millis();
    }
    scanning = true;
  }

  // Тривога
  if (currentRSSI > RSSI_THRESHOLD && millis() > alarmEnd) {
    digitalWrite(BUZZER_PIN, HIGH);
    alarmEnd = millis() + ALARM_DURATION;
    Serial.printf("ALARM! %s Ch%d RSSI:%d dBm\n",
      bandName(currentBand), currentCh, currentRSSI);
  }
  if (millis() > alarmEnd) digitalWrite(BUZZER_PIN, LOW);
}

void updateDisplay() {
  if (millis() - lastUI < UI_UPDATE_INTERVAL) return;
  lastUI = millis();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // Режим
  const char* modes[] = {"MONITOR", "SCAN   ", "DETECTOR"};
  u8g2.drawStr(0, 10, modes[mode]);

  // Бенд + канал + RSSI
  char line[30];
  sprintf(line, "%s Ch:%02d %c", bandName(currentBand), currentCh,
          scanning ? '>' : ' ');
  u8g2.drawStr(0, 25, line);

  sprintf(line, "RSSI: %d dBm", currentRSSI);
  u8g2.drawStr(0, 40, line);

  if (bestRSSI > -95) {
    sprintf(line, "Best: %d @ Ch%02d", bestRSSI, bestCh);
    u8g2.drawStr(0, 55, line);
  }

  // Бар RSSI
  int bar = constrain((currentRSSI + 100) * 128 / 70, 0, 128);
  u8g2.drawBox(0, 58, bar, 6);

  u8g2.sendBuffer();
}

// Допоміжні функції
uint8_t getChannelCount() {
  switch (currentBand) {
    case B12G: return 8;
    case B33G: return 64;
    case B58G: return 48;
  }
  return 8;
}

const char* bandName(Band b) {
  const char* names[] = {"1.2GHz", "3.3GHz", "5.8GHz"};
  return names[b];
}

void setChannel(Band b, uint8_t ch) {
  switch (b) {
    case B12G: sm12.setChannel(ch); break;
    case B33G: ft33.setChannel(ch); break;
    case B58G: rx58.setChannel(ch); break;
  }
}

int readRSSI(Band b) {
  switch (b) {
    case B12G: return sm12.getRSSI();
    case B33G: return ft33.getRSSI();
    case B58G: return map(rx58.getRSSI(), 0, 4095, -95, -25);
  }
  return -100;
}