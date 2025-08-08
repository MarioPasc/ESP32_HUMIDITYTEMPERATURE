#pragma once

#include <Arduino.h>

namespace cfg {
    // GPIOs
    constexpr gpio_num_t PIN_DHT      = GPIO_NUM_5;
    constexpr uint8_t    I2C_SDA      = 21;
    constexpr uint8_t    I2C_SCL      = 22;

    // I²C & display
    constexpr uint8_t    OLED_ADDR    = 0x3C;
    constexpr uint16_t   OLED_W       = 128;
    constexpr uint16_t   OLED_H       = 64;

    // Task timing
    constexpr TickType_t SENSOR_PERIOD = pdMS_TO_TICKS(2000); // DHT11 max 0.5 Hz
    constexpr TickType_t UI_PERIOD     = pdMS_TO_TICKS(500);  // smoother refresh

    // WiFi credentials - CHANGE THESE!
    extern const char* WIFI_SSID;
    extern const char* WIFI_PASS;
    
    // Server endpoint - CHANGE THIS to your computer's IP!
    extern const char* SERVER_URL;
    
    // Data collection
    constexpr size_t READINGS_PER_BATCH = 10;
    constexpr size_t NETWORK_QUEUE_SIZE = 5;
} // namespace cfg
