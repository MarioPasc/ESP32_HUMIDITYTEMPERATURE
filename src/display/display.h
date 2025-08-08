#pragma once

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config/config.h"
#include "types/types.h"

class Display {
private:
    Adafruit_SSD1306& oled;

public:
    explicit Display(Adafruit_SSD1306& oled_ref);
    
    bool init();
    void centrePrint(int16_t y, const String& txt);
    void showWiFiStatus(const String& status);
    void showSensorData(const Reading& reading, size_t buffer_count, bool wifi_connected);
    void showError(const String& error);
    void clear();
    void display();
};
