#include "sensor.h"
#include "config/config.h"
#include <esp_log.h>

static const char* TAG = "SENSOR";

SensorManager::SensorManager() : dht(cfg::PIN_DHT, DHT11), reading_mutex(nullptr) {}

SensorManager::~SensorManager() {
    if (reading_mutex) {
        vSemaphoreDelete(reading_mutex);
    }
}

bool SensorManager::init() {
    // GPIO setup
    pinMode(cfg::PIN_DHT, INPUT_PULLUP);
    delay(1000);
    
    // Initialize DHT sensor
    ESP_LOGI(TAG, "Initializing DHT sensor...");
    dht.begin();
    delay(2000);
    
    // Create mutex
    reading_mutex = xSemaphoreCreateMutex();
    if (!reading_mutex) {
        ESP_LOGE(TAG, "Failed to create reading mutex");
        return false;
    }
    
    ESP_LOGI(TAG, "Sensor manager initialized");
    return true;
}

Reading SensorManager::getCurrentReading() {
    Reading reading;
    
    if (reading_mutex) {
        xSemaphoreTake(reading_mutex, portMAX_DELAY);
        reading = current_reading;
        xSemaphoreGive(reading_mutex);
    }
    
    return reading;
}

bool SensorManager::readSensor(Reading& reading) {
    sensors_event_t evT, evH;
    
    ESP_LOGD(TAG, "Attempting to read DHT sensor...");
    
    dht.temperature().getEvent(&evT);
    dht.humidity().getEvent(&evH);

    bool temp_valid = !isnan(evT.temperature);
    bool hum_valid = !isnan(evH.relative_humidity);
    
    if (temp_valid && hum_valid) {
        reading.t = evT.temperature;
        reading.h = evH.relative_humidity;
        reading.timestamp = millis();
        
        // Update current reading
        if (reading_mutex) {
            xSemaphoreTake(reading_mutex, portMAX_DELAY);
            current_reading = reading;
            xSemaphoreGive(reading_mutex);
        }
        
        ESP_LOGI(TAG, "Sensor reading: %.1f °C, %.0f %%RH", reading.t, reading.h);
        return true;
    } else {
        ESP_LOGW(TAG, "Invalid sensor reading - temp_valid: %d, hum_valid: %d", temp_valid, hum_valid);
        return false;
    }
}
