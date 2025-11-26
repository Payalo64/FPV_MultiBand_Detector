#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>

#include "FT3500.h"
#include "Bands.h"

// ================== ПІНИ ESP32-S3 (ПРИКЛАД, МОЖНА ПІДРЕДАГУВАТИ) ==================
// Кнопки
const int PIN_BTN_UP     = 18;
const int PIN_BTN_DOWN   = 19;
const int PIN_BTN_SELECT = 20;

// Біпер
const int PIN_BEEP = 21;

// Відео мультиплексор (CD4053)
const int PIN_VID_SEL0 = 16;
const int PIN_VID_SEL1 = 17;

// RSSI входи (ADC1)
const int PIN_RSSI_12 = 1;  // ADC1_CH0
const int PIN_RSSI_33 = 2;  // ADC1_CH1
const int PIN_RSSI_58 = 3;  // ADC1_CH2

// Піни керування SM1370R (1.2 ГГц) – 4 біти каналу
const int PIN_SM_CH0 = 6;
const int PIN_SM_CH1 = 7;
const int PIN_SM_CH2 = 14;
const int PIN_SM_CH3 = 15;

// Піни FT3500 (3.3 ГГц) – як у твоєму класі FT3500
// ПРИКЛАД – підстав свої реальні
const int PIN_FT_S1  = 22;
const int PIN_FT_S2  = 23;
const int PIN_FT_S3  = 24;
const int PIN_FT_CS1 = 25;
const int PIN_FT_CS2 = 26;
const int PIN_FT_CS3 = 27;

// SPI для RX5808 (якщо будеш робити SPI-керування)
// Можна не використовувати, якщо RX5808 у тебе в паралельному режимі.
const int PIN_SPI_SCK  = 12;
const int PIN_SPI_MOSI = 11;
const int PIN_SPI_MISO = 13;
const int PIN_CS_RX5808 = 4;

// I2C OLED
const int PIN_I2C_SCL = 8;
const int PIN_I2C_SDA = 9;

// ================== OLED (U8G2) ==================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ================== СТРУКТУРИ ==================
struct Band {
  const char*   name;        // "1.2G", "3.3G", "5.8G"
  uint8_t       receiverId;  // 0,1,2
  const uint16_t* freqsMHz;  // масив частот
  uint8_t       channelCount;
  uint16_t      rssiMin;     // шум (калібровка)
  uint16_t      rssiMax;     // максимум (калібровка)
};

struct Receiver {
  void     (*setChannel)(uint8_t ch);
  uint16_t (*getRSSI_raw)();
};

struct PeakInfo {
  uint8_t  bandIndex;
  uint8_t  channelIndex;
  uint16_t rssiRaw;
  uint8_t  rssiPercent;
};

// ================== GLOBAL ==================
Receiver receivers[3];

Band bands[3] = {
  { "1.2G", 0, freqs_12G,    SM1370R_CHANNEL_COUNT,  200,  900 },
  { "3.3G", 1, freqs_33G,    FT3500_CHANNEL_COUNT,   200,  900 },
  { "5.8G", 2, freqs_58G,    RX5808_CHANNEL_COUNT,   200,  900 }
};

uint8_t currentBand    = 0;  // 0=1.2G, 1=3.3G, 2=5.8G
uint8_t currentChannel = 0;

enum UIMode { UI_MONITOR, UI_SCAN, UI_DETECTOR };
UIMode uiMode = UI_MONITOR;

// Кнопки
bool lastUp = false, lastDown = false, lastSelect = false;
uint32_t selectPressTime = 0;

// Детектор
const uint8_t  DETECT_THRESHOLD_PERCENT   = 70;     // поріг тривоги (0–100%)
const uint32_t ALARM_ON_TIME_MS           = 800;    // триматись вище порогу
const uint32_t ALARM_OFF_TIME_MS          = 1500;   // триматись нижче, щоб скинути
bool     alarmActive      = false;
uint32_t alarmStartTime   = 0;
uint32_t alarmStopTime    = 0;

// Для auto-вибору
const uint8_t  SWITCH_DELTA_PERCENT       = 5;      // наскільки сильніший має бути сигнал
const uint32_t MIN_SWITCH_INTERVAL_MS     = 500;    // мін. інтервал між переключеннями
PeakInfo activeSource = {0, 0, 0, 0};               // те, що реально зараз виводиться
PeakInfo lastPeak    = {0, 0, 0, 0};
uint32_t lastSwitchTime = 0;

// ================== ДРАЙВЕРИ ПРИЙМАЧІВ ==================
// ---- SM1370R (1.2 GHz) ----
void sm1370_setChannel(uint8_t ch) {
  if (ch >= SM1370R_CHANNEL_COUNT) return;
  digitalWrite(PIN_SM_CH0, (ch & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_SM_CH1, (ch & 0x02) ? HIGH : LOW);
  digitalWrite(PIN_SM_CH2, (ch & 0x04) ? HIGH : LOW);
  digitalWrite(PIN_SM_CH3, (ch & 0x08) ? HIGH : LOW);
  delay(5);
}

uint16_t sm1370_getRSSI_raw() {
  return analogRead(PIN_RSSI_12);
}

// ---- FT3500 (3.3 GHz) ----
// Глобальний об'єкт FT3500 з твоїми пін-ами
FT3500 ft3500(PIN_FT_S1, PIN_FT_S2, PIN_FT_S3,
              PIN_FT_CS1, PIN_FT_CS2, PIN_FT_CS3);

inline void ft3500_indexToFrCh(uint8_t idx, uint8_t &fr, uint8_t &ch) {
  fr = idx / FT3500_CH_PER_FR; // 0..7
  ch = idx % FT3500_CH_PER_FR; // 0..7
}

void ft3500_setChannel_flat(uint8_t idx) {
  if (idx >= FT3500_CHANNEL_COUNT) return;
  uint8_t fr, ch;
  ft3500_indexToFrCh(idx, fr, ch);
  ft3500.setChannel(fr, ch);
}

uint16_t ft3500_getRSSI_raw() {
  return analogRead(PIN_RSSI_33);
}

// ---- RX5808 (5.8 GHz) ----
// Тут спрощений варіант: ти вставиш свою реалізацію з rx5808-pro-diversity
// Зараз лише "заглушка", яка НІЧОГО не посилає по SPI, але дозволяє зібрати прошивку.
void rx58_setChannel(uint8_t idx) {
  if (idx >= RX5808_CHANNEL_COUNT) return;
  uint16_t freq = freqs_58G[idx];
  // TODO: ВСТАВ сюди свій код керування RX5808 по SPI
  // (наприклад, з проекту rx5808-pro-diversity)
  (void)freq; // щоб компілятор не сварився
}

uint16_t rx58_getRSSI_raw() {
  return analogRead(PIN_RSSI_58);
}

// ================== ВІДЕО МУЛЬТИПЛЕКСОР ==================
void selectVideoSource(uint8_t bandIndex) {
  switch (bandIndex) {
    case 0: // 1.2G
      digitalWrite(PIN_VID_SEL0, LOW);
      digitalWrite(PIN_VID_SEL1, LOW);
      break;
    case 1: // 3.3G
      digitalWrite(PIN_VID_SEL0, HIGH);
      digitalWrite(PIN_VID_SEL1, LOW);
      break;
    case 2: // 5.8G
    default:
      digitalWrite(PIN_VID_SEL0, LOW);
      digitalWrite(PIN_VID_SEL1, HIGH);
      break;
  }
}

// ================== RSSI: ФІЛЬТРАЦІЯ ТА НОРМАЛІЗАЦІЯ ==================
uint16_t getRSSIavg_raw(uint8_t receiverId, uint8_t samples = 8) {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < samples; i++) {
    acc += receivers[receiverId].getRSSI_raw();
    delayMicroseconds(500);
  }
  return acc / samples;
}

uint8_t rssiRawToPercent(uint16_t raw, const Band& band) {
  if (raw <= band.rssiMin) return 0;
  if (raw >= band.rssiMax) return 100;
  return (uint8_t)((raw - band.rssiMin) * 100UL / (band.rssiMax - band.rssiMin));
}

// ================== КНОПКИ ==================
bool readButton(int pin) {
  return digitalRead(pin) == LOW; // кнопка на GND, INPUT_PULLUP
}

void handleButtons() {
  bool up    = readButton(PIN_BTN_UP);
  bool down  = readButton(PIN_BTN_DOWN);
  bool sel   = readButton(PIN_BTN_SELECT);

  // Up
  if (up && !lastUp) {
    if (uiMode == UI_MONITOR) {
      currentChannel++;
      if (currentChannel >= bands[currentBand].channelCount) currentChannel = 0;
      receivers[bands[currentBand].receiverId].setChannel(currentChannel);
      selectVideoSource(currentBand);
      tone(PIN_BEEP, 2000, 40);
    } else if (uiMode == UI_SCAN) {
      currentBand++;
      if (currentBand > 2) currentBand = 0;
      currentChannel = 0;
      receivers[bands[currentBand].receiverId].setChannel(currentChannel);
      selectVideoSource(currentBand);
      tone(PIN_BEEP, 1800, 40);
    }
  }

  // Down
  if (down && !lastDown) {
    if (uiMode == UI_MONITOR) {
      if (currentChannel == 0) currentChannel = bands[currentBand].channelCount - 1;
      else currentChannel--;
      receivers[bands[currentBand].receiverId].setChannel(currentChannel);
      selectVideoSource(currentBand);
      tone(PIN_BEEP, 1500, 40);
    } else if (uiMode == UI_SCAN) {
      if (currentBand == 0) currentBand = 2;
      else currentBand--;
      currentChannel = 0;
      receivers[bands[currentBand].receiverId].setChannel(currentChannel);
      selectVideoSource(currentBand);
      tone(PIN_BEEP, 1300, 40);
    }
  }

  // Select: коротке – перемикаємо режими
  if (sel && !lastSelect) {
    selectPressTime = millis();
  }
  if (!sel && lastSelect) {
    uint32_t dt = millis() - selectPressTime;
    if (dt < 1000) {
      if (uiMode == UI_MONITOR) uiMode = UI_SCAN;
      else if (uiMode == UI_SCAN) uiMode = UI_DETECTOR;
      else uiMode = UI_MONITOR;
      tone(PIN_BEEP, 2500, 60);
    } else {
      // довге натискання – тут можна зробити Setup Menu в майбутньому
    }
  }

  lastUp     = up;
  lastDown   = down;
  lastSelect = sel;
}

// ================== OLED UI ==================
void drawMonitorScreen() {
  const Band& b = bands[currentBand];
  uint16_t rssiRaw = getRSSIavg_raw(b.receiverId);
  uint8_t  rssiPct = rssiRawToPercent(rssiRaw, b);
  uint16_t freq    = b.freqsMHz[currentChannel];

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  u8g2.setCursor(0, 10);
  u8g2.print("MONITOR ");
  u8g2.print(b.name);

  u8g2.setCursor(0, 24);
  u8g2.print("CH:");
  u8g2.print(currentChannel);
  u8g2.print("  ");
  u8g2.print(freq);
  u8g2.print(" MHz");

  u8g2.setCursor(0, 38);
  u8g2.print("RSSI: ");
  u8g2.print(rssiPct);
  u8g2.print("%");

  int barWidth = map(rssiPct, 0, 100, 0, 120);
  u8g2.drawFrame(0, 48, 120, 12);
  u8g2.drawBox(0, 48, barWidth, 12);

  u8g2.sendBuffer();
}

void drawScanScreen() {
  const Band& b = bands[currentBand];

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setCursor(0, 10);
  u8g2.print("SCAN ");
  u8g2.print(b.name);

  uint8_t maxPct = 0;
  uint8_t maxCh  = 0;

  uint8_t maxBars = 16;
  uint8_t step    = max((uint8_t)1, (uint8_t)(b.channelCount / maxBars));
  uint8_t x       = 0;

  for (uint8_t ch = 0; ch < b.channelCount; ch += step) {
    receivers[b.receiverId].setChannel(ch);
    selectVideoSource(currentBand);
    delay(3);
    uint16_t raw = getRSSIavg_raw(b.receiverId, 4);
    uint8_t  pct = rssiRawToPercent(raw, b);
    if (pct > maxPct) {
      maxPct = pct;
      maxCh  = ch;
    }
    uint8_t barH = map(pct, 0, 100, 0, 40);
    u8g2.drawBox(x, 64 - barH, 4, barH);
    x += 6;
  }

  u8g2.setCursor(0, 24);
  u8g2.print("Best CH:");
  u8g2.print(maxCh);
  u8g2.print(" ");
  u8g2.print(maxPct);
  u8g2.print("%");

  u8g2.sendBuffer();

  lastPeak.bandIndex    = currentBand;
  lastPeak.channelIndex = maxCh;
  lastPeak.rssiPercent  = maxPct;
  lastPeak.rssiRaw      = 0;
}

void drawDetectorScreen(const PeakInfo& peak, bool alarm) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  u8g2.setCursor(0, 10);
  u8g2.print("DETECTOR");

  const Band& b = bands[peak.bandIndex];
  uint16_t freq = b.freqsMHz[peak.channelIndex];

  u8g2.setCursor(0, 24);
  u8g2.print("BAND: ");
  u8g2.print(b.name);

  u8g2.setCursor(0, 36);
  u8g2.print("CH:");
  u8g2.print(peak.channelIndex);
  u8g2.print("  ");
  u8g2.print(freq);
  u8g2.print(" MHz");

  u8g2.setCursor(0, 48);
  u8g2.print("RSSI: ");
  u8g2.print(peak.rssiPercent);
  u8g2.print("%");

  if (alarm) {
    u8g2.setFont(u8g2_font_6x13B_tr);
    u8g2.setCursor(0, 64);
    u8g2.print("!! ALERT FPV/DRONE !!");
  }

  u8g2.sendBuffer();
}

// ================== DETECTOR: MULTI-BAND SCAN ==================
void runDetector() {
  PeakInfo peak = {0, 0, 0, 0};

  // 1) Скануємо всі 3 діапазони
  for (uint8_t bIndex = 0; bIndex < 3; bIndex++) {
    const Band& b = bands[bIndex];
    for (uint8_t ch = 0; ch < b.channelCount; ch++) {
      receivers[b.receiverId].setChannel(ch);
      selectVideoSource(bIndex);
      delay(3);

      uint16_t raw = getRSSIavg_raw(b.receiverId, 3);
      uint8_t  pct = rssiRawToPercent(raw, b);

      if (pct > peak.rssiPercent) {
        peak.bandIndex    = bIndex;
        peak.channelIndex = ch;
        peak.rssiRaw      = raw;
        peak.rssiPercent  = pct;
      }
    }
  }

  lastPeak = peak;

  // 2) Вирішуємо, чи переключати активне джерело
  uint32_t now = millis();
  bool shouldSwitch = false;

  if (activeSource.rssiPercent == 0 && activeSource.rssiRaw == 0) {
    shouldSwitch = true;
  } else {
    if (peak.rssiPercent > activeSource.rssiPercent + SWITCH_DELTA_PERCENT) {
      if (now - lastSwitchTime > MIN_SWITCH_INTERVAL_MS) {
        shouldSwitch = true;
      }
    }
  }

  if (shouldSwitch) {
    activeSource = peak;
    lastSwitchTime = now;

    currentBand    = activeSource.bandIndex;
    currentChannel = activeSource.channelIndex;

    const Band& b = bands[currentBand];
    receivers[b.receiverId].setChannel(currentChannel);
    selectVideoSource(currentBand);
  }

  // 3) Тривога по activeSource
  uint8_t refRssi = activeSource.rssiPercent;
  uint32_t now2   = millis();

  if (!alarmActive) {
    if (refRssi >= DETECT_THRESHOLD_PERCENT) {
      if (alarmStartTime == 0) alarmStartTime = now2;
      if ((now2 - alarmStartTime) >= ALARM_ON_TIME_MS) {
        alarmActive = true;
        alarmStopTime = 0;
        tone(PIN_BEEP, 3000, 200);
      }
    } else {
      alarmStartTime = 0;
    }
  } else {
    if (refRssi < DETECT_THRESHOLD_PERCENT) {
      if (alarmStopTime == 0) alarmStopTime = now2;
      if ((now2 - alarmStopTime) >= ALARM_OFF_TIME_MS) {
        alarmActive = false;
        alarmStartTime = 0;
      }
    } else {
      alarmStopTime = 0;
    }
  }

  // 4) Малюємо екран по активному джерелу
  drawDetectorScreen(activeSource, alarmActive);
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN_UP,     INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN,   INPUT_PULLUP);
  pinMode(PIN_BTN_SELECT, INPUT_PULLUP);

  pinMode(PIN_BEEP, OUTPUT);
  pinMode(PIN_VID_SEL0, OUTPUT);
  pinMode(PIN_VID_SEL1, OUTPUT);

  pinMode(PIN_SM_CH0, OUTPUT);
  pinMode(PIN_SM_CH1, OUTPUT);
  pinMode(PIN_SM_CH2, OUTPUT);
  pinMode(PIN_SM_CH3, OUTPUT);

  pinMode(PIN_FT_S1,  OUTPUT);
  pinMode(PIN_FT_S2,  OUTPUT);
  pinMode(PIN_FT_S3,  OUTPUT);
  pinMode(PIN_FT_CS1, OUTPUT);
  pinMode(PIN_FT_CS2, OUTPUT);
  pinMode(PIN_FT_CS3, OUTPUT);

  pinMode(PIN_CS_RX5808, OUTPUT);
  digitalWrite(PIN_CS_RX5808, HIGH);

  // SPI для RX5808 (якщо будеш використовувати)
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

  // I2C + OLED
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  u8g2.begin();

  // Прив'язуємо драйвери
  receivers[0].setChannel  = sm1370_setChannel;
  receivers[0].getRSSI_raw = sm1370_getRSSI_raw;

  receivers[1].setChannel  = ft3500_setChannel_flat;
  receivers[1].getRSSI_raw = ft3500_getRSSI_raw;

  receivers[2].setChannel  = rx58_setChannel;
  receivers[2].getRSSI_raw = rx58_getRSSI_raw;

  currentBand    = 0;
  currentChannel = 0;
  receivers[bands[currentBand].receiverId].setChannel(currentChannel);
  selectVideoSource(currentBand);

  tone(PIN_BEEP, 2000, 100);
}

void loop() {
  handleButtons();

  switch (uiMode) {
    case UI_MONITOR:
      drawMonitorScreen();
      break;
    case UI_SCAN:
      drawScanScreen();
      break;
    case UI_DETECTOR:
      runDetector();
      break;
  }
}