FPV MultiBand Detector v3.0 FINAL
Мультидіапазонний детектор FPV-дронів (1.2 / 3.3 / 5.8 GHz)
Повністю переписана, стабільна та максимально швидка прошивка 2025 року
� � � �
Можливості
Одночасна підтримка трьох діапазонів:
1.2 GHz (SM1370R / VM1373R) – 8 каналів
3.3 GHz (FT3500) – 64 канали
5.8 GHz (RX5808 з SPI-модом) – 48 каналів (A/B/E/F/R/L + RaceBand)
3 режими роботи:
MONITOR – ручний вибір каналу
SCAN – швидке сканування поточного діапазону
DETECTOR – повний циклічний скан усіх трьох діапазонів
Non-blocking сканування (кнопки та дисплей завжди реагують)
OLED 128×64 (SSD1306, I²C) з графіком RSSI
Звукова + світлова тривога при перевищенні порогу (налаштовується)
Wi-Fi та Bluetooth вимкнені автоматично – мінімальний шум на 5.8 GHz
Watchdog, захист від зависань, калібровані значення RSSI
Усі піни та налаштування зібрані в config.h – легко адаптувати під свою збірку
Схема підключення (ESP32-S3 DevKit)
Пристрій
Пін ESP32-S3
Примітка
OLED SDA
GPIO 8

OLED SCL
GPIO 9

Кнопка UP
GPIO 0
(BOOT) – внутрішня підтяжка
Кнопка DOWN
GPIO 1

Кнопка MODE
GPIO 3

Buzzer / LED
GPIO 2

SM1370R A/B/C
4 / 5 / 6
3-біт вибір каналу
SM1370R RSSI
GPIO 17
Аналоговий вхід
FT3500 CS
GPIO 10
SPI
FT3500 RSSI
GPIO 18
Аналоговий вхід
RX5808 CS
GPIO 5
SPI (обов’язковий SPI-mod)
RX5808 RSSI
GPIO 34
(вхід-only, ADC1_CH6)
Якщо використовуєте іншу плату або інші піни – редагуйте тільки config.h
Встановлення та прошивка
Встановіть Arduino IDE 2.x
Додайте підтримку ESP32:
https://raw.githubusercontent.com/espressif/arduino-esp32/master/package_esp32_index.json
Встановіть бібліотеку U8g2 (через Library Manager)
Створіть новий скетч → додайте 6 вкладок → вставте файли з цього репозиторію:
FPV_MultiBand_Detector_v3.ino
config.h
Bands.h
RX5808.h
FT3500.h
SM1370R.h
Налаштування компіляції:
Board: ESP32S3 Dev Module
Partition Scheme: Huge APP (3MB No OTA)
Upload Speed: 921600
Підключіть плату → Upload
Готово! Пристрій стартує в режимі MONITOR (5.8 GHz).
Керування
Кнопка
Дія
MODE
Перемикання режимів (MONITOR → SCAN → DETECTOR)
UP
Наступний канал / вибір діапазону
DOWN
Попередній канал
Налаштування (config.h)
#define RSSI_THRESHOLD     -68   // поріг тривоги, dBm
#define SCAN_DWELL_TIME    35    // мс на канал (чим менше – швидше)
#define ALARM_DURATION     1500  // мс
Скріншоти дисплея (приклад)
DETECTOR
5.8GHz Ch:23 >
RSSI: -52 dBm
Best: -48 @ Ch19
[███████████████        ]   ← графік RSSI
Подяка
Оригінальний репозиторій: Payalo64
Бібліотека EZ-FPV (натхнення для RX5808-драйвера)
Всі учасники спільноти FPV Ukraine та Reddit/r/Multicopter
Ліцензія
MIT – використовуйте, модифікуйте та поширюйте вільно.
Гарних польотів і вдалого виявлення дронів!
27.11.2025