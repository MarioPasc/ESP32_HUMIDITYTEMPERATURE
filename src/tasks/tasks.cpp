#include "tasks.h"
#include "config/config.h"
#include <esp_log.h>

static const char* TAG = "TASKS";

TaskManager::TaskManager(SensorManager& sensor, NetworkManager& network, Display& disp)
    : sensor_manager(sensor), network_manager(network), display(disp), buffer_mutex(nullptr) {}

TaskManager::~TaskManager() {
    if (buffer_mutex) {
        vSemaphoreDelete(buffer_mutex);
    }
}

bool TaskManager::init() {
    buffer_mutex = xSemaphoreCreateMutex();
    if (!buffer_mutex) {
        ESP_LOGE(TAG, "Failed to create buffer mutex");
        return false;
    }
    
    ESP_LOGI(TAG, "Task manager initialized");
    return true;
}

void TaskManager::sensorTask() {
    Reading reading;
    
    for (;;) {
        if (sensor_manager.readSensor(reading)) {
            // Add to buffer for batch sending
            xSemaphoreTake(buffer_mutex, portMAX_DELAY);
            readings_buffer.push_back(reading);
            
            // Check if we have enough readings to send
            if (readings_buffer.size() >= cfg::READINGS_PER_BATCH) {
                ReadingBatch batch = readings_buffer;   // copy
                
                // Try to send without clearing first (as per your request)
                bool ok = network_manager.queueBatch(batch);
                if (ok) {
                    readings_buffer.clear();
                    ESP_LOGI(TAG, "Batch queued successfully, buffer cleared");
                } else {
                    ESP_LOGW(TAG, "Failed to queue batch, keeping data in buffer");
                }
            }
            xSemaphoreGive(buffer_mutex);

            ESP_LOGI(TAG, "Sampled: %.1f °C  %.0f %%RH (Buffer: %d/%d)", 
                     reading.t, reading.h, readings_buffer.size(), cfg::READINGS_PER_BATCH);
        }

        vTaskDelay(cfg::SENSOR_PERIOD);
    }
}

void TaskManager::uiTask() {
    for (;;) {
        Reading r = sensor_manager.getCurrentReading();
        size_t buffer_count;
        
        xSemaphoreTake(buffer_mutex, portMAX_DELAY);
        buffer_count = readings_buffer.size();
        xSemaphoreGive(buffer_mutex);

        display.showSensorData(r, buffer_count, network_manager.isConnected());
        
        vTaskDelay(cfg::UI_PERIOD);
    }
}

void TaskManager::sensorTaskWrapper(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    manager->sensorTask();
}

void TaskManager::uiTaskWrapper(void* param) {
    TaskManager* manager = static_cast<TaskManager*>(param);
    manager->uiTask();
}
