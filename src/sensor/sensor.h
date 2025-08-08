#pragma once

#include <Arduino.h>
#include <DHT_U.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "types/types.h"

class SensorManager {
private:
    DHT_Unified dht;
    Reading current_reading;
    SemaphoreHandle_t reading_mutex;
    
public:
    SensorManager();
    ~SensorManager();
    
    bool init();
    Reading getCurrentReading();
    bool readSensor(Reading& reading);
};
