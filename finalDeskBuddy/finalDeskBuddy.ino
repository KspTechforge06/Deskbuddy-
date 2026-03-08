/*
 ╔══════════════════════════════════════════════════════╗
 ║          DESK BUDDY v1.0  –  ESP8266 + OLED          ║
 ║  Features: Animated Eyes, Clock, Temp, Weather API,  ║
 ║            Touch Button, Modes, Expressions          ║
 ╚══════════════════════════════════════════════════════╝

 Hardware:
  - ESP8266 (NodeMCU / Wemos D1 Mini)
  - SSD1306 128x64 OLED  (I2C: SDA=D2/GPIO4, SCL=D1/GPIO5)
  - DHT11 or DHT22        (DATA=D4/GPIO2)
  - Tactile button        (D5/GPIO14, other leg to GND)

 Libraries (install via Arduino Library Manager):
  - Adafruit SSD1306
  - Adafruit GFX
  - DHT sensor library (Adafruit)
  - ArduinoJson  (v6)
  - NTPClient by Fabrice Weinberg
  - ESP8266 board core (includes WiFi, HTTPClient, WiFiUDP)
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>               // part of ESP8266 core
#include <NTPClient.h>
#include <ArduinoJson.h>

// ─── USER CONFIG ──────────────────────────────────────
const char* WIFI_SSID      = "Unwanted Galaxy";
const char* WIFI_PASS      = "@pranavk";
const char* CITY           = "Pune";   // used in weather + display
const long  UTC_OFFSET_SEC = 19800;      // IST = +5:30
// ──────────────────────────────────────────────────────

// ─── PIN DEFINITIONS (GPIO numbers) ───────────────────
#define OLED_SDA    4    // D2
#define OLED_SCL    5    // D1
#define DHT_PIN     2    // D4
#define DHT_TYPE    DHT11
#define BTN_PIN     14   // D5
// ──────────────────────────────────────────────────────

// ─── OLED ─────────────────────────────────────────────
#define SCREEN_W 128
#define SCREEN_H  64
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ─── DHT ──────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ─── NTP ──────────────────────────────────────────────
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_SEC, 60000);

// ─── MODES & EXPRESSIONS ──────────────────────────────
enum Mode       { MODE_EYES, MODE_CLOCK, MODE_WEATHER, MODE_TEMP, MODE_COUNT };
enum Expression { EXP_NORMAL, EXP_HAPPY, EXP_SLEEPY, EXP_SURPRISED, EXP_COUNT };

Mode       currentMode = MODE_EYES;
Expression currentExpr = EXP_NORMAL;

// ─── EYE STATE ────────────────────────────────────────
float eyeX = 64, eyeY = 32;
int   eyeTargetX = 64, eyeTargetY = 32;
bool  blinking   = false;
int   blinkFrame = 0;
unsigned long lastBlink      = 0;
unsigned long lastEyeMove    = 0;
unsigned long lastExprChange = 0;

// ─── WEATHER CACHE ────────────────────────────────────
String weatherDesc  = "Loading...";
float  weatherTemp  = 0;
int    weatherHumid = 0;
unsigned long lastWeatherFetch = 0;

// ─── SENSOR ───────────────────────────────────────────
float roomTemp  = 0;
float roomHumid = 0;
unsigned long lastSensorRead = 0;

// ─── BUTTON ───────────────────────────────────────────
bool          btnLastState = HIGH;
unsigned long btnDebounce  = 0;

// ══════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed! Check wiring.");
    while (true) delay(100);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  dht.begin();
  bootScreen();

  // ── Connect WiFi ──────────────────────────────────
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 24);
  display.print("Connecting WiFi");
  display.display();

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    tries++;
    display.print(".");
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    timeClient.begin();
    timeClient.update();
    fetchWeather();
  } else {
    Serial.println("\nWiFi failed — running offline");
    display.clearDisplay();
    display.setCursor(10, 24);
    display.print("WiFi failed!");
    display.setCursor(10, 36);
    display.print("Offline mode");
    display.display();
    delay(2000);
  }

  lastBlink      = millis();
  lastEyeMove    = millis();
  lastExprChange = millis();
}

// ══════════════════════════════════════════════════════
void loop() {
  handleButton();

  // Read sensor every 3s
  if (millis() - lastSensorRead > 3000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) roomTemp  = t;
    if (!isnan(h)) roomHumid = h;
    lastSensorRead = millis();
  }

  // NTP + weather refresh
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    if (millis() - lastWeatherFetch > 600000UL) {
      fetchWeather();
    }
  }

  // Auto-cycle expressions every 8s
  if (millis() - lastExprChange > 8000) {
    currentExpr = (Expression)((currentExpr + 1) % EXP_COUNT);
    lastExprChange = millis();
  }

  display.clearDisplay();
  switch (currentMode) {
    case MODE_EYES:    drawEyes();       break;
    case MODE_CLOCK:   drawClock();      break;
    case MODE_WEATHER: drawWeather();    break;
    case MODE_TEMP:    drawTempScreen(); break;
    default: break;
  }
  display.display();
  delay(30);
}

// ──────────────────────────────────────────────────────
void handleButton() {
  bool state = digitalRead(BTN_PIN);
  if (state == LOW && btnLastState == HIGH && millis() - btnDebounce > 200) {
    currentMode = (Mode)((currentMode + 1) % MODE_COUNT);
    btnDebounce = millis();
    Serial.print("Mode: "); Serial.println(currentMode);
  }
  btnLastState = state;
}

// ══════════════════════════════════════════════════════
//  BOOT SCREEN
// ══════════════════════════════════════════════════════
void bootScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 8);
  display.print("Desk");
  display.setCursor(10, 30);
  display.print("Buddy!");
  display.setTextSize(1);
  display.setCursor(30, 54);
  display.print("v1.0  PRANAV");
  display.display();
  delay(2000);
}

// ══════════════════════════════════════════════════════
//  ANIMATED EYES
// ══════════════════════════════════════════════════════
void drawEyes() {
  // Smooth pupil movement
  if (millis() - lastEyeMove > 2000) {
    eyeTargetX = random(52, 76);
    eyeTargetY = random(24, 40);
    lastEyeMove = millis();
  }
  eyeX += (eyeTargetX - eyeX) * 0.15f;
  eyeY += (eyeTargetY - eyeY) * 0.15f;

  // Blink logic
  if (!blinking && millis() - lastBlink > (unsigned long)random(3000, 6000)) {
    blinking   = true;
    blinkFrame = 0;
    lastBlink  = millis();
  }
  if (blinking) {
    blinkFrame++;
    if (blinkFrame > 10) { blinking = false; blinkFrame = 0; }
  }
  int openH = blinking ? max(0, 22 - blinkFrame * 5) : 22;

  switch (currentExpr) {
    case EXP_NORMAL:    drawNormalEyes(openH); break;
    case EXP_HAPPY:     drawHappyEyes();       break;
    case EXP_SLEEPY:    drawSleepyEyes();      break;
    case EXP_SURPRISED: drawSurprisedEyes();   break;
  }

  // Mini clock bottom-center
  display.setTextSize(1);
  display.setCursor(44, 56);
  display.print(getTimeStr());
}

void drawNormalEyes(int openH) {
  if (openH < 2) return;
  int px = (int)eyeX, py = (int)eyeY;

  display.drawRoundRect(18, 20, 38, openH, 8, WHITE);
  display.drawRoundRect(72, 20, 38, openH, 8, WHITE);

  if (openH > 8) {
    // Left pupil
    display.fillCircle(px - 27, py, 6, WHITE);
    display.fillCircle(px - 29, py - 2, 2, BLACK);   // glint
    // Right pupil
    display.fillCircle(px + 27, py, 6, WHITE);
    display.fillCircle(px + 25, py - 2, 2, BLACK);
  }
}

void drawHappyEyes() {
  for (int i = 0; i <= 180; i += 4) {
    float r = i * PI / 180.0f;
    display.drawPixel(37 + (int)(16 * cos(r)), 30 - (int)(10 * sin(r)), WHITE);
    display.drawPixel(91 + (int)(16 * cos(r)), 30 - (int)(10 * sin(r)), WHITE);
  }
  // Smile
  for (int i = 200; i <= 340; i += 4) {
    float r = i * PI / 180.0f;
    display.drawPixel(64 + (int)(20 * cos(r)), 50 - (int)(8 * sin(r)), WHITE);
  }
}

void drawSleepyEyes() {
  display.fillRect(18, 29, 38, 11, WHITE);
  display.fillRect(72, 29, 38, 11, WHITE);
  display.fillRect(18, 20, 38,  9, BLACK);
  display.fillRect(72, 20, 38,  9, BLACK);
  display.setTextSize(1);
  display.setCursor(100, 10); display.print("z");
  display.setCursor(108,  4); display.print("z");
}

void drawSurprisedEyes() {
  display.drawCircle(37, 30, 16, WHITE);
  display.drawCircle(91, 30, 16, WHITE);
  display.fillCircle(37, 30,  7, WHITE);
  display.fillCircle(91, 30,  7, WHITE);
  display.fillCircle(37, 30,  3, BLACK);
  display.fillCircle(91, 30,  3, BLACK);
  display.fillRect(61, 46, 4, 10, WHITE);
  display.fillRect(61, 58, 4,  4, WHITE);
}

// ══════════════════════════════════════════════════════
//  CLOCK SCREEN
// ══════════════════════════════════════════════════════
void drawClock() {
  String t = getTimeStr();
  String d = getDateStr();

  display.setTextSize(3);
  int tw = t.length() * 18;
  display.setCursor((SCREEN_W - tw) / 2, 12);
  display.print(t);

  display.drawFastHLine(10, 46, 108, WHITE);

  display.setTextSize(1);
  int dw = d.length() * 6;
  display.setCursor((SCREEN_W - dw) / 2, 52);
  display.print(d);
}

// ══════════════════════════════════════════════════════
//  WEATHER SCREEN
// ══════════════════════════════════════════════════════
void drawWeather() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(CITY);
  display.print("  ");
  display.print(getTimeStr());
  display.drawFastHLine(0, 10, 128, WHITE);

  display.setTextSize(3);
  display.setCursor(0, 16);
  display.print((int)weatherTemp);

  display.setTextSize(1);
  display.setCursor(54, 16);
  display.print((char)247);
  display.print("C");

  display.setCursor(80, 26);
  display.print("H:");
  display.print(weatherHumid);
  display.print("%");

  display.setTextSize(1);
  display.setCursor(0, 50);
  String desc = weatherDesc;
  if (desc.length() > 21) desc = desc.substring(0, 21);
  display.print(desc);
}

// ══════════════════════════════════════════════════════
//  ROOM SENSOR SCREEN
// ══════════════════════════════════════════════════════
void drawTempScreen() {
  display.setTextSize(1);
  display.setCursor(24, 0);
  display.print("ROOM SENSOR");
  display.drawFastHLine(0, 10, 128, WHITE);

  if (roomTemp == 0 && roomHumid == 0) {
    display.setCursor(20, 30);
    display.print("Reading...");
    return;
  }

  display.setTextSize(2);
  display.setCursor(4, 16);
  display.print("T:");
  display.print(roomTemp, 1);
  display.setTextSize(1);
  display.print((char)247);
  display.print("C");

  display.setTextSize(2);
  display.setCursor(4, 38);
  display.print("H:");
  display.print(roomHumid, 0);
  display.print("%");

  float hi = dht.computeHeatIndex(roomTemp, roomHumid, false);
  display.setTextSize(1);
  display.setCursor(0, 57);
  display.print("Feels:");
  display.print(hi, 1);
  display.print((char)247);
}

// ══════════════════════════════════════════════════════
//  HELPERS
// ══════════════════════════════════════════════════════
String getTimeStr() {
  if (WiFi.status() != WL_CONNECTED) return "--:--";
  char buf[6];
  sprintf(buf, "%02d:%02d", timeClient.getHours(), timeClient.getMinutes());
  return String(buf);
}

String getDateStr() {
  unsigned long ep = timeClient.getEpochTime();
  time_t t = (time_t)ep;
  struct tm* tm_info = gmtime(&t);
  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  const char* mons[] = {"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"};
  char buf[20];
  sprintf(buf, "%s %d %s %d",
    days[tm_info->tm_wday],
    tm_info->tm_mday,
    mons[tm_info->tm_mon],
    1900 + tm_info->tm_year);
  return String(buf);
}

// ══════════════════════════════════════════════════════
//  WEATHER FETCH — wttr.in (no API key needed)
// ══════════════════════════════════════════════════════
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;

  String url = "http://wttr.in/";
  url += String(CITY);
  url += "?format=j1";

  Serial.println("Fetching: " + url);
  http.begin(client, url);
  http.setTimeout(8000);    // wttr.in can be slow, give it 8s

  int code = http.GET();
  Serial.print("HTTP code: "); Serial.println(code);

  if (code == 200) {
    DynamicJsonDocument doc(4096);   // larger buffer for wttr.in response
    DeserializationError err = deserializeJson(doc, http.getStream());
    if (!err) {
      weatherTemp  = doc["current_condition"][0]["temp_C"].as<float>();
      weatherHumid = doc["current_condition"][0]["humidity"].as<int>();
      weatherDesc  = doc["current_condition"][0]["weatherDesc"][0]["value"].as<String>();
      Serial.println("Weather OK: " + weatherDesc);
    } else {
      Serial.print("JSON error: "); Serial.println(err.c_str());
      weatherDesc = "Parse error";
    }
  } else {
    Serial.println("HTTP failed");
    weatherDesc = "Fetch failed";
  }

  http.end();
  lastWeatherFetch = millis();
}
