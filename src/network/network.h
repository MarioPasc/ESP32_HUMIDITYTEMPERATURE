#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "types/types.h"

class NetworkManager {
private:
    QueueHandle_t batch_queue;
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool init();
    bool connectWiFi();
    bool isConnected() const;
    String getLocalIP() const;
    
    bool queueBatch(const ReadingBatch& batch);
    bool sendDataBatch(const ReadingBatch& readings);
    
    void networkTask();
    
    static void networkTaskWrapper(void* param);
};
