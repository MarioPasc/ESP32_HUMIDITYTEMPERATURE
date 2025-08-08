/*
 * esp32_temp_hum.cpp  —  Display DHT11 readings on a 0.96" I²C SSD1306 + WiFi JSON sender
 * 
 * Build: PlatformIO, env = esp32dev, framework = arduino, C++17
 * 
 * ┌── Electrical map ───────────────────────────────────────────────┐
 * │ OLED SDA → GPIO21   OLED SCL → GPIO22   OLED VCC → 3V3  GND ↔   │
 * │ DHT11 DATA → GPIO5                  DHT VCC → 3V3   GND ↔       │
 * └──────────────────────────────────────────────────────────────────┘
 */

#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Local modules
#include "config/config.h"
#include "types/types.h"
#include "display/display.h"
#include "sensor/sensor.h"
#include "network/network.h"
#include "tasks/tasks.h"

// ───────────────────────────── Module instances ─────────────────────────────
static const char *TAG = "APP";

static TwoWire I2CBus = TwoWire(0);  // dedicated bus instance
static Adafruit_SSD1306 oled(cfg::OLED_W, cfg::OLED_H, &I2CBus, -1);

static Display display(oled);
static SensorManager sensor_manager;
static NetworkManager network_manager;
static TaskManager task_manager(sensor_manager, network_manager, display);

// ───────────────────────────── Helper functions ──────────────────────────────
[[noreturn]] static void fatal(const char *msg)
{
    ESP_LOGE(TAG, "%s", msg);
    while (true) { vTaskDelay(portMAX_DELAY); }
}

// ───────────────────────────── Arduino lifecycle ─────────────────────────────
void setup()
{
    // Serial & logging
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "=== ESP32 Environmental Monitor Starting ===");

    // I²C
    I2CBus.begin(cfg::I2C_SDA, cfg::I2C_SCL, 400000);

    // Display initialization
    if (!display.init()) {
        fatal("Display initialization failed");
    }

    // Show WiFi connection status on display
    display.showWiFiStatus("Connecting...");

    // Network initialization
    if (!network_manager.init()) {
        fatal("Network manager initialization failed");
    }

    // WiFi connection with display feedback
    bool wifi_connected = network_manager.connectWiFi();
    if (wifi_connected) {
        display.showWiFiStatus("Connected: " + network_manager.getLocalIP());
        delay(2000); // Show IP for 2 seconds
    } else {
        display.showWiFiStatus("Failed");
        delay(2000);
    }

    // Sensor initialization
    if (!sensor_manager.init()) {
        fatal("Sensor manager initialization failed");
    }

    // Task manager initialization
    if (!task_manager.init()) {
        fatal("Task manager initialization failed");
    }

    ESP_LOGI(TAG, "Creating tasks...");

    // Create tasks
    xTaskCreatePinnedToCore(TaskManager::sensorTaskWrapper, "SensorTask", 8192, &task_manager, 1, nullptr, 1);
    xTaskCreatePinnedToCore(TaskManager::uiTaskWrapper, "UiTask", 4096, &task_manager, 1, nullptr, 1);
    xTaskCreatePinnedToCore(NetworkManager::networkTaskWrapper, "NetTask", 8192, &network_manager, 1, nullptr, 1);

    ESP_LOGI(TAG, "=== Initialization Complete ===");
}

void loop() { 
    vTaskDelete(nullptr); 
}