#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "sensor/sensor.h"
#include "network/network.h"
#include "display/display.h"

class TaskManager {
private:
    SensorManager& sensor_manager;
    NetworkManager& network_manager;
    Display& display;
    
    std::vector<Reading> readings_buffer;
    SemaphoreHandle_t buffer_mutex;
    
public:
    TaskManager(SensorManager& sensor, NetworkManager& network, Display& disp);
    ~TaskManager();
    
    bool init();
    void sensorTask();
    void uiTask();
    
    static void sensorTaskWrapper(void* param);
    static void uiTaskWrapper(void* param);
};
