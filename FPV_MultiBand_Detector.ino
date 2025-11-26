/*
 * FPV MultiBand Detector v2.0 - Виправлена версія
 * Автор: Grok (на основі Payalo64)
 * Дата: 27.11.2025
 * Опис: Мультибенд-сканер FPV-сигналів (1.2/3.3/5.8 GHz) з non-blocking логікою.
 * Режими: MONITOR (ручний), SCAN (авто в бенді), DETECTOR (повний скан).
 * Залежності: U8g2, ESP32 core.
 */

#include <WiFi.h>  // Для відключення інтерференції
#include <U8g2lib.h>  // OLED
#include "Bands.h"  // Таблиці частот
#include "FT3500.h"  // Драйвер 3.3G

// Enum для бендів (виправлення: замість #define)
enum Band { BAND_12GHZ, BAND_33GHZ, BAND_58GHZ, BAND_COUNT };

// Піни (адаптовано для ESP32-S3)
#define OLED_SDA 8
#define OLED_SCL 9
#define BUZZER_PIN 2
#define BTN_UP 0
#define BTN_DOWN 1
#define BTN_MODE 3

// Для 1.2G (SM1370R/VM1373R: 3-біт GPIO)
#define CH1_12G 4
#define CH2_12G 5
#define CH3_12G 6

// Для 5.8G (RX5808: GPIO для каналу)
#define CH_SEL_58G 18

// Для 3.3G (FT3500: SPI)
#define FT_CS 10

// OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Глобальні змінні
Band currentBand = BAND_58GHZ;  // Старт з 5.8G
int currentChannel = 0;
int rssi = -100;
int rssiThreshold = -70;  // Тривога при > -70 dBm
unsigned long lastScanTime = 0;
unsigned long scanInterval = 50;  // ms на канал
bool inScanMode = false;
int scanProgress = 0;
int maxRssi = -100;
int maxChannel = 0;
int mode = 0;  // 0: MONITOR, 1: SCAN, 2: DETECTOR

// Таймери (non-blocking)
unsigned long lastUpdate = 0;
unsigned long updateInterval = 100;  // ms для UI
unsigned long alarmTime = 0;
bool alarmActive = false;

void setup() {
  Serial.begin(115200);
  Serial.println("FPV MultiBand Detector v2.0 starting...");

  // Відключення Wi-Fi/BT для зменшення шуму
  WiFi.mode(WIFI_OFF);
  btStop();

  // Ініціалізація пінів
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(CH_SEL_58G, OUTPUT);
  pinMode(CH1_12G, OUTPUT);
  pinMode(CH2_12G, OUTPUT);
  pinMode(CH3_12G, OUTPUT);
  digitalWrite(CH_SEL_58G, LOW);
  digitalWrite(CH1_12G, LOW);
  digitalWrite(CH2_12G, LOW);
  digitalWrite(CH3_12G, LOW);

  // SPI для FT3500
  SPI.begin(FT_CS, /*miso*/12, /*mosi*/11, /*sck*/13);  // VSPI default

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!u8g2.begin()) {
    Serial.println("OLED init failed!");
    while(1);
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "FPV Detector v2.0");
  u8g2.sendBuffer();

  // Watchdog (виправлення: стабільність)
  esp_task_wdt_init(30, true);  // 30s timeout
  esp_task_wdt_add(NULL);

  Serial.println("Setup complete.");
}

void loop() {
  esp_task_wdt_feed();  // Feed watchdog

  // Читання кнопок (debounce простий)
  if (digitalRead(BTN_MODE) == LOW) {
    mode = (mode + 1) % 3;
    inScanMode = false;
    delay(200);  // Simple debounce
  }
  if (digitalRead(BTN_UP) == LOW) {
    currentChannel = (currentChannel + 1) % getChannelCount(currentBand);
    delay(200);
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    currentChannel = (currentChannel - 1 + getChannelCount(currentBand)) % getChannelCount(currentBand);
    delay(200);
  }

  // Логіка режимів
  switch (mode) {
    case 0:  // MONITOR
      setChannel(currentBand, currentChannel);
      rssi = readRSSI(currentBand);
      break;
    case 1:  // SCAN (в поточному бенді, non-blocking)
      if (millis() - lastScanTime > scanInterval) {
        setChannel(currentBand, scanProgress);
        rssi = readRSSI(currentBand);
        if (rssi > maxRssi) {
          maxRssi = rssi;
          maxChannel = scanProgress;
        }
        scanProgress++;
        if (scanProgress >= getChannelCount(currentBand)) {
          currentChannel = maxChannel;
          inScanMode = false;
          scanProgress = 0;
          maxRssi = -100;
          Serial.printf("Scan complete: Max RSSI %d on ch %d\n", maxRssi, maxChannel);
        }
        lastScanTime = millis();
      }
      if (!inScanMode && digitalRead(BTN_MODE) != LOW) inScanMode = true;  // Start scan
      break;
    case 2:  // DETECTOR (всі бенди, non-blocking)
      if (millis() - lastScanTime > scanInterval) {
        setChannel(currentBand, currentChannel);
        rssi = readRSSI(currentBand);
        if (rssi > maxRssi) {
          maxRssi = rssi;
          maxChannel = currentChannel;
          // Перехід на бенд
        }
        currentChannel++;
        if (currentChannel >= getChannelCount(currentBand)) {
          currentChannel = 0;
          currentBand = (Band)((int)currentBand + 1) % BAND_COUNT;
        }
        if (/* full scan done */) {  // TODO: track full cycle
          inScanMode = false;
        }
        lastScanTime = millis();
      }
      if (!inScanMode) inScanMode = true;
      break;
  }

  // Тривога (якщо RSSI > threshold)
  if (rssi > rssiThreshold && !alarmActive) {
    alarmActive = true;
    alarmTime = millis();
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("ALARM: Strong signal detected!");
  }
  if (alarmActive && millis() - alarmTime > 1000) {
    alarmActive = false;
    digitalWrite(BUZZER_PIN, LOW);
  }

  // UI оновлення (non-blocking)
  if (millis() - lastUpdate > updateInterval) {
    updateDisplay();
    lastUpdate = millis();
  }
}

// Встановлення каналу (загальна)
void setChannel(Band band, int ch) {
  switch (band) {
    case BAND_12GHZ:
      setChannel12G(ch);  // GPIO bits
      break;
    case BAND_33GHZ:
      setChannelFT3500(ch);
      break;
    case BAND_58GHZ:
      setChannelRX5808(ch);
      break;
  }
  Serial.printf("Set band %d, ch %d\n", band, ch);
}

// Читання RSSI (з валідацією)
int readRSSI(Band band) {
  int rawRssi;
  switch (band) {
    case BAND_12GHZ:
      rawRssi = analogRead(CH1_12G);  // Приклад; адаптуйте для модуля
      break;
    case BAND_33GHZ:
      rawRssi = getRSSI_FT3500();
      break;
    case BAND_58GHZ:
      rawRssi = getRSSI_RX5808();
      break;
    default:
      rawRssi = -100;
  }
  // Валідація (виправлення)
  if (rawRssi < -100 || rawRssi > 0) rawRssi = -100;
  return rawRssi;
}

// Кількість каналів
int getChannelCount(Band band) {
  switch (band) {
    case BAND_12GHZ: return 8;
    case BAND_33GHZ: return 64;
    case BAND_58GHZ: return 48;
  }
  return 1;
}

// Оновлення дисплея
void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // Заголовок
  char modeStr[20];
  switch (mode) { case 0: strcpy(modeStr, "MONITOR"); case 1: strcpy(modeStr, "SCAN"); case 2: strcpy(modeStr, "DETECTOR"); }
  u8g2.drawStr(0, 10, modeStr);

  // Бенд та канал
  char bandStr[10] = {"1.2G", "3.3G", "5.8G"}[(int)currentBand];
  char chStr[20];
  sprintf(chStr, "%s Ch:%d RSSI:%d", bandStr, currentChannel, rssi);
  u8g2.drawStr(0, 25, chStr);

  // Прогрес скану (бар)
  if (inScanMode) {
    int barWidth = (scanProgress * 128) / getChannelCount(currentBand);
    u8g2.drawFrame(0, 35, 128, 10);
    u8g2.drawBox(0, 35, barWidth, 10);
  }

  // RSSI графік (простий)
  int graphHeight = (rssi + 100) * 20 / 50;  // Scale -100..-50 to 0..20
  u8g2.drawFrame(0, 50, 128, 14);
  u8g2.drawBox(0, 64 - graphHeight, 128, graphHeight);

  u8g2.sendBuffer();
}

// Драйвери модулів (заглушки/виправлені)
void setChannel12G(int ch) {
  // 3-біт: ch 0-7
  digitalWrite(CH1_12G, (ch & 1) ? HIGH : LOW);
  digitalWrite(CH2_12G, (ch & 2) ? HIGH : LOW);
  digitalWrite(CH3_12G, (ch & 4) ? HIGH : LOW);
}

void setChannelRX5808(int ch) {
  // Простий GPIO для RX5808 (адаптуйте для SPI якщо mod)
  digitalWrite(CH_SEL_58G, HIGH);
  delayMicroseconds(10);  // Pulse
  digitalWrite(CH_SEL_58G, LOW);
  // Повний SPI: див. EZ-FPV для кращого
}

int getRSSI_RX5808() {
  // Заглушка; реально - analogRead або SPI read reg 0x1C
  return analogRead(17) - 200;  // Приклад offset
}