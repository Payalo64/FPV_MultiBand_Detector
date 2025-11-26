# FPV MultiBand Detector v3.0 FINAL
**Найпотужніший мультидіапазонний детектор FPV-дронів 2025 року**  
1.2 GHz | 3.3 GHz | 5.8 GHz  

![Version](https://img.shields.io/badge/Version-3.0_FINAL-brightgreen?style=for-the-badge)
![ESP32-S3](https://img.shields.io/badge/ESP32--S3-Ready-blue?style=for-the-badge)
![Non-blocking](https://img.shields.io/badge/Non--blocking-Yes-success?style=for-the-badge)
![Open Source](https://img.shields.io/badge/Open_Source-Yes-ff69b4?style=for-the-badge)

Повністю переписана, оптимізована та стабільна прошивка на базі ESP32-S3  
Готова до заливки «з коробки» — просто скопіюй 6 файлів і проший!

## Можливості
- 3 діапазони одночасно:
  - 1.2 GHz (SM1370R / VM1373R) — 8 каналів
  - 3.3 GHz (FT3500) — 64 канали
  - 5.8 GHz (RX5808 + SPI-mod) — 48 каналів (A/B/E/F/R/L + RaceBand)
- 3 режими роботи:
  - **MONITOR** — ручний вибір каналу
  - **SCAN** — швидке сканування поточного діапазону
  - **DETECTOR** — безперервний пошук по всіх трьох діапазонах
- Non-blocking сканування — кнопки та дисплей завжди реагують
- OLED 128×64 (SSD1306) з графіком RSSI та інфолініями
- Звукова + світлова тривога при сильному сигналі
- Автоматичне відключення Wi-Fi/Bluetooth — мінімальні перешкоди
- Вбудований watchdog, захист від зависань
- Усі піни та налаштування в одному файлі `config.h`

## Підключення (ESP32-S3 DevKit)

| Пристрій              | Пін ESP32-S3 | Примітка                          |
|-----------------------|--------------|-----------------------------------|
| OLED SDA              | GPIO 8       |                                   |
| OLED SCL              | GPIO 9       |                                   |
| Кнопка UP             | GPIO 0       | (BOOT) — внутрішня підтяжка      |
| Кнопка DOWN           | GPIO 1       |                                   |
| Кнопка MODE           | GPIO 3       |                                   |
| Buzzer / LED          | GPIO 2       |                                   |
| SM1370R A / B / C     | 4 / 5 / 6    | 3-біт вибір каналу                |
| SM1370R RSSI          | GPIO 17      | Аналоговий вхід                   |
| FT3500 CS             | GPIO 10      | SPI                               |
| FT3500 RSSI           | GPIO 18      | Аналоговий вхід                   |
| RX5808 CS             | GPIO 5       | Обов’язковий SPI-mod!             |
| RX5808 RSSI           | GPIO 34      | (input-only, ADC1_CH6)            |

> Змінити піни? — редагуй лише `config.h`

## Встановлення та прошивка

1. Встанови Arduino IDE 2.x  
2. Додай підтримку ESP32:  
   `https://raw.githubusercontent.com/espressif/arduino-esp32/master/package_esp32_index.json`
3. Встанови бібліотеку **U8g2** (через Library Manager)
4. Створи новий скетч → додай 6 вкладок → встав файли з репозиторію:
   - `FPV_MultiBand_Detector_v3.ino`
   - `config.h`
   - `Bands.h`
   - `RX5808.h`
   - `FT3500.h`
   - `SM1370R.h`
5. Налаштування:
   - Board: **ESP32S3 Dev Module**
   - Partition Scheme: **Huge APP (3MB No OTA)**
   - Upload Speed: **921600**
6. Проший — готово!

## Керування

| Кнопка  | Дія                                     |
|---------|-----------------------------------------|
| MODE    | Перемикання MONITOR → SCAN → DETECTOR   |
| UP      | Наступний канал / діапазон              |
| DOWN    | Попередній канал                        |

## Налаштування (config.h)

#define RSSI_THRESHOLD     -68   // поріг тривоги (dBm)
#define SCAN_DWELL_TIME    35    // мс на канал (чим менше — швидше)
#define ALARM_DURATION     1500  // тривалість звуку (мс)

## Приклад екрану

```DETECTOR
5.8GHz Ch:23 >
RSSI: -52 dBm
Best: -48 @ Ch19
[███████████████        ]

```MONITOR
1.2GHz Ch:05
RSSI: -91 dBm
[██                      ]

```SCAN
3.3GHz Ch:47 >
RSSI: -34 dBm
Best: -34 @ Ch47
[████████████████████████]

## Подяка

Оригінальний проєкт: Payalo64
Натхнення та драйвери: EZ-FPV, Shea Ivey (RX5808 SPI-mod), спільноти FPV Ukraine, r/Multicopter, r/fpv
Тестування та фідбек: вся українська FPV-спільнота 2024–2025
Ліцензія
MIT License — використовуй, модифікуй, поширюй вільно!
Гарних польотів і точного виявлення дронів!
27 листопада 2025