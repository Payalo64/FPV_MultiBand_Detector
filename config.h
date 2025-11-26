// config.h
#ifndef CONFIG_H
#define CONFIG_H

// === OLED ===
#define OLED_SDA      8
#define OLED_SCL      9
#define OLED_ADDR     0x3C

// === Кнопки ===
#define BTN_UP        0   // GPIO0 (BOOT) — підтяжка внутрішня
#define BTN_DOWN      1
#define BTN_MODE      3

// === Buzzer / LED ===
#define BUZZER_PIN    2
#define ALARM_LED     2   // той самий пін

// === 1.2 GHz — SM1370R / VM1373R ===
#define PIN_12G_A     4   // біт 0
#define PIN_12G_B     5   // біт 1
#define PIN_12G_C     6   // біт 2
#define PIN_RSSI_12G  17  // аналоговий вхід RSSI

// === 3.3 GHz — FT3500 ===
#define FT_CS         10
#define FT_RSSI_PIN   18  // аналоговий вхід RSSI від модуля

// === 5.8 GHz — RX5808 (SPI mod) ===
#define RX5808_CS     5
#define RX5808_RSSI   34  // ADC1_CH6 (вхід тільки)

// === Налаштування сканера ===
#define RSSI_THRESHOLD     -68   // dBm — поріг тривоги
#define SCAN_DWELL_TIME    35    // мс на канал (швидко і точно)
#define ALARM_DURATION     1500  // мс
#define UI_UPDATE_INTERVAL 120   // мс

#endif