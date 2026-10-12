// ============================================================
// Wetterstation CYD ESP32-3248S035R
// UI erzeugt mit EEZ Studio (LVGL 9.5)
// Prilchen LABS 2026
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <esp_sleep.h>
#include "driver/rtc_io.h"
#include "secrets.h"

#define LV_LVGL_H_INCLUDE_SIMPLE
#include "ui.h"

const char *apiUrl = "http://api.open-meteo.com/v1/forecast?latitude=50.91&longitude=6.81&current=temperature_2m,relative_humidity_2m,wind_speed_10m,wind_direction_10m&hourly=precipitation_probability&forecast_days=1&timezone=Europe/Berlin";

#define TFT_HOR_RES 480
#define TFT_VER_RES 320
#define TEMP_MIN (-20.0f)
#define TEMP_MAX (50.0f)
#define NEEDLE_LENGTH 100
#define TOUCH_IRQ_PIN 36
#define SCREEN_TIMEOUT_MS 60000

TFT_eSPI tft = TFT_eSPI();
static lv_color_t draw_buf[TFT_HOR_RES * 10];
unsigned long lastTime = 0;
const unsigned long timerDelay = 900000;
unsigned long screenOnTime = 0;

bool fetchAndDisplayWeather();

static const char *wind_dir_text(int deg) {
  deg = ((deg % 360) + 360) % 360;
  if (deg < 22 || deg >= 338) return "N";
  if (deg < 67) return "NO";
  if (deg < 112) return "O";
  if (deg < 157) return "SO";
  if (deg < 202) return "S";
  if (deg < 247) return "SW";
  if (deg < 292) return "W";
  return "NW";
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t breite = lv_area_get_width(area);
  uint32_t hoehe = lv_area_get_height(area);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, breite, hoehe);
  tft.pushColors((uint16_t *)px_map, breite * hoehe, true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

static void update_gauge(float temp) {
  if (temp < TEMP_MIN) temp = TEMP_MIN;
  if (temp > TEMP_MAX) temp = TEMP_MAX;
  lv_scale_set_line_needle_value(objects.scale_main, objects.scale_needle,
                                 NEEDLE_LENGTH, (int32_t)temp);
  lv_label_set_text_fmt(objects.label_temp, "%.1f C", temp);
}

static void goToDeepSleep() {
  Serial.println("Gehe in Deep Sleep - Touch zum Aufwachen");
  digitalWrite(27, LOW);
  tft.writecommand(0x10);
  delay(120);
  pinMode(22, INPUT);
  pinMode(16, INPUT);
  pinMode(17, INPUT);
  pinMode(TOUCH_IRQ_PIN, INPUT);
  unsigned long t0 = millis();
  while (digitalRead(TOUCH_IRQ_PIN) == LOW && millis() - t0 < 5000) delay(50);
  delay(200);
  rtc_gpio_pullup_en((gpio_num_t)TOUCH_IRQ_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)TOUCH_IRQ_PIN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_IRQ_PIN, 0);
  esp_deep_sleep_start();
}

static void update_clock_from_meteo(const String &isoTime) {
  if (isoTime.length() < 16) {
    lv_label_set_text(objects.label_time, "--.--.---- --:--");
    return;
  }
  String jahr = isoTime.substring(0, 4);
  String monat = isoTime.substring(5, 7);
  String tag = isoTime.substring(8, 10);
  String stunde = isoTime.substring(11, 13);
  String minute = isoTime.substring(14, 16);
  lv_label_set_text_fmt(objects.label_time, "%s.%s.%s %s:%s", tag.c_str(),
                        monat.c_str(), jahr.c_str(), stunde.c_str(), minute.c_str());
}

void setup() {
  Serial.begin(115200);
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Aufgewacht durch Touch"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Aufgewacht durch Timer"); break;
    case ESP_SLEEP_WAKEUP_UNDEFINED: Serial.println("Kaltstart / Reset"); break;
    default: Serial.println("Anderer Grund");
  }

  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);
  tft.begin();
  tft.setRotation(1);

  lv_init();
  lv_tick_set_cb(millis);
  lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);
  ui_init();

  lv_label_set_text(objects.label_status, "WLAN...");
  lv_obj_set_style_text_color(objects.label_status, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_timer_handler();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lv_timer_handler();
  }
  Serial.println(" verbunden");

  lv_label_set_text(objects.label_status, "online");
  lv_obj_set_style_text_color(objects.label_status, lv_color_hex(0x13B594), LV_PART_MAIN);
  lv_timer_handler();
  fetchAndDisplayWeather();
  screenOnTime = millis();
  lastTime = millis();
}

void loop() {
  lv_timer_handler();
  delay(5);
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) fetchAndDisplayWeather();
    lastTime = millis();
  }
  if (screenOnTime > 0 && (millis() - screenOnTime) > SCREEN_TIMEOUT_MS) goToDeepSleep();
}

bool fetchAndDisplayWeather() {
  for (int versuch = 1; versuch <= 3; versuch++) {
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(apiUrl);
    int code = http.GET();
    if (code == 200) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        float temp = doc["current"]["temperature_2m"];
        int hum = doc["current"]["relative_humidity_2m"];
        float wind = doc["current"]["wind_speed_10m"];
        int winddir = doc["current"]["wind_direction_10m"];
        String nowStr = doc["current"]["time"].as<String>();
        update_clock_from_meteo(nowStr);
        int rain = 0;
        if (nowStr.length() >= 13) {
          int hour = nowStr.substring(11, 13).toInt();
          if (hour >= 0 && hour < 24) rain = doc["hourly"]["precipitation_probability"][hour];
        }
        update_gauge(temp);
        lv_label_set_text_fmt(objects.label_hum, "Luftfeuchtigkeit: %d%%", hum);
        lv_label_set_text_fmt(objects.label_wind, "Wind: %.0f km/h %s", wind, wind_dir_text(winddir));
        lv_label_set_text_fmt(objects.label_rain, "Regenrisiko: %d%%", rain);
        Serial.printf("OK: T=%.1f H=%d%% Wind=%.0f %s Regen=%d%% Zeit=%s\n",
                      temp, hum, wind, wind_dir_text(winddir), rain, nowStr.c_str());
        http.end();
        return true;
      }
      Serial.print("JSON-Fehler: ");
      Serial.println(error.c_str());
    } else {
      Serial.printf("HTTP-Code %d (Versuch %d/3)\n", code, versuch);
    }
    http.end();
    delay(800);
  }
  lv_label_set_text(objects.label_status, "API-Fehler");
  lv_obj_set_style_text_color(objects.label_status, lv_color_hex(0xFF5252), LV_PART_MAIN);
  return false;
}
