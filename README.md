# 🤖 Desk Buddy — ESP8266 OLED Desktop Companion
![preview](image_source)

A cute animated desktop buddy built on ESP8266 with a 128×64 OLED display. Features animated expressive eyes, live clock via NTP, real-time weather (no API key needed), room temperature/humidity sensor, and a single button to cycle through all screens.

![Platform](https://img.shields.io/badge/platform-ESP8266-blue)
![IDE](https://img.shields.io/badge/IDE-Arduino-teal)
![License](https://img.shields.io/badge/license-MIT-green)

---

## ✨ Features

- 👁️ **Animated Eyes** — blinking, roaming pupils, 4 expressions (Normal, Happy, Sleepy, Surprised)
- 🕐 **Clock Screen** — live time and date via NTP (IST / any timezone)
- 🌤️ **Weather Screen** — temperature, humidity, description via [wttr.in](https://wttr.in) *(no API key needed)*
- 🌡️ **Room Sensor Screen** — live DHT11/DHT22 temp, humidity, and heat index
- 🔘 **Single Button** — cycles through all screens
- 📶 **Offline Mode** — works without WiFi (shows sensor data and "--:--" for time)

---

## 🛒 Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | ESP8266 (NodeMCU v3 or Wemos D1 Mini) |
| Display | SSD1306 128×64 OLED (I2C) |
| Temp Sensor | DHT11 or DHT22 |
| Button | Tactile push button or TTP223 touch module |

---

## 🔌 Wiring

| Component | ESP8266 Pin | GPIO |
|-----------|------------|------|
| OLED SDA | D2 | GPIO 4 |
| OLED SCL | D1 | GPIO 5 |
| DHT DATA | D4 | GPIO 2 |
| Button | D5 | GPIO 14 |
| Button (other leg) | GND | — |

> OLED VCC → 3.3V, OLED GND → GND  
> DHT VCC → 3.3V, DHT GND → GND (add 10kΩ pull-up on DATA pin for DHT22)

---

## 📚 Libraries

Install all of these via **Arduino IDE → Library Manager**:

| Library | Author |
|---------|--------|
| Adafruit SSD1306 | Adafruit |
| Adafruit GFX | Adafruit |
| DHT sensor library | Adafruit |
| ArduinoJson (v6) | Benoit Blanchon |
| NTPClient | Fabrice Weinberg |

> The ESP8266 board core (WiFi, HTTPClient, WiFiUdp) is bundled with the board package.

### Installing the ESP8266 Board Package

1. Arduino IDE → **File → Preferences**
2. Add to *Additional Boards Manager URLs*:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. **Tools → Board → Boards Manager** → search `esp8266` → Install **"esp8266 by ESP8266 Community"**

---

## ⚙️ Configuration

Open `DeskBuddy.ino` and edit these lines at the top:

```cpp
const char* WIFI_SSID      = "your_wifi_name";
const char* WIFI_PASS      = "your_wifi_password";
const char* CITY           = "Mumbai";   // your city for weather
const long  UTC_OFFSET_SEC = 19800;      // IST = 19800 | UTC = 0 | EST = -18000
```

> **No API key needed.** Weather is fetched from [wttr.in](https://wttr.in) — a free, open-source weather service.

---

## 🚀 Flashing

1. Clone this repo or download `DeskBuddy.ino`
2. Open in **Arduino IDE**
3. Select board: **Tools → Board → NodeMCU 1.0** (or your ESP8266 variant)
4. Select the correct **Port**
5. Click **Upload**

---

## 🖥️ Screens

| Screen | What it shows |
|--------|--------------|
| **Eyes** | Animated face with blinking, roaming pupils, 4 auto-cycling expressions + mini clock |
| **Clock** | Large HH:MM display with full date (NTP synced) |
| **Weather** | Outdoor temp °C, humidity %, weather description (updates every 10 min) |
| **Sensor** | Room temp, humidity, and feels-like heat index from DHT |

Press the button once to move to the next screen.

---


## 📁 File Structure

```
DeskBuddy/
└── DeskBuddy.ino    # Main sketch — everything is in one file
```

---

## 🙏 Credits

- Weather data by [wttr.in](https://wttr.in) — Igor Chubin
- Time sync via [NTP Pool Project](https://www.ntppool.org)
- Built by **KSP**

---

## 📄 License

MIT License — free to use, modify, and share.
