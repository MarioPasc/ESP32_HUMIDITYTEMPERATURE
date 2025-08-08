#include "display.h"
#include <esp_log.h>

static const char* TAG = "DISPLAY";

Display::Display(Adafruit_SSD1306& oled_ref) : oled(oled_ref) {}

bool Display::init() {
    if (!oled.begin(SSD1306_SWITCHCAPVCC, cfg::OLED_ADDR)) {
        ESP_LOGE(TAG, "SSD1306 init failed");
        return false;
    }
    
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    centrePrint(0, F("Environmental + WiFi"));
    oled.display();
    
    ESP_LOGI(TAG, "Display initialized successfully");
    return true;
}

void Display::centrePrint(int16_t y, const String& txt) {
    int16_t x1, y1;
    uint16_t w, h;
    oled.getTextBounds(txt.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t x = (cfg::OLED_W - w) / 2;
    oled.setCursor(x, y);
    oled.print(txt);
}

void Display::showWiFiStatus(const String& status) {
    oled.fillRect(0, 16, cfg::OLED_W, 32, SSD1306_BLACK);
    centrePrint(24, "WiFi: " + status);
    oled.display();
}

void Display::showSensorData(const Reading& reading, size_t buffer_count, bool wifi_connected) {
    oled.fillRect(0, 16, cfg::OLED_W, 48, SSD1306_BLACK);

    if (isnan(reading.t) || isnan(reading.h)) {
        centrePrint(32, F("Sensor error"));
    } else {
        centrePrint(20, "T = " + String(reading.t, 1) + " C");
        centrePrint(32, "RH = " + String(reading.h, 0) + " %");
        
        // Show WiFi status and buffer count
        String status = "WiFi: " + String(wifi_connected ? "OK" : "X") + 
                       " Buf: " + String(buffer_count) + "/" + String(cfg::READINGS_PER_BATCH);
        centrePrint(44, status);
    }
    oled.display();
}

void Display::showError(const String& error) {
    oled.fillRect(0, 16, cfg::OLED_W, 48, SSD1306_BLACK);
    centrePrint(32, error);
    oled.display();
}

void Display::clear() {
    oled.clearDisplay();
}

void Display::display() {
    oled.display();
}
