#include "network.h"
#include "config/config.h"
#include <esp_log.h>

static const char* TAG = "NETWORK";

NetworkManager::NetworkManager() : batch_queue(nullptr) {}

NetworkManager::~NetworkManager() {
    if (batch_queue) {
        vQueueDelete(batch_queue);
    }
}

bool NetworkManager::init() {
    batch_queue = xQueueCreate(cfg::NETWORK_QUEUE_SIZE, sizeof(ReadingBatch*));
    if (!batch_queue) {
        ESP_LOGE(TAG, "Failed to create network queue");
        return false;
    }
    
    ESP_LOGI(TAG, "Network manager initialized");
    return true;
}

bool NetworkManager::connectWiFi() {
    // Disconnect any existing connection
    WiFi.disconnect(true);
    delay(1000);
    
    ESP_LOGI(TAG, "Starting WiFi connection...");
    ESP_LOGI(TAG, "SSID: %s", cfg::WIFI_SSID);
    ESP_LOGI(TAG, "Password length: %d", strlen(cfg::WIFI_PASS));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg::WIFI_SSID, cfg::WIFI_PASS);
    ESP_LOGI(TAG, "Connecting to WiFi...");
    
    int attempts = 0;
    const int max_attempts = 40; // Increased to 20 seconds
    
    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        attempts++;
        
        // Show connection status every 5 attempts
        if (attempts % 5 == 0) {
            ESP_LOGI(TAG, "WiFi connection attempt %d/%d, Status: %d", 
                     attempts, max_attempts, WiFi.status());
        }
        
        // Try to reconnect every 10 attempts
        if (attempts % 10 == 0 && attempts < max_attempts) {
            ESP_LOGI(TAG, "Retrying WiFi connection...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(cfg::WIFI_SSID, cfg::WIFI_PASS);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi connected successfully!");
        ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "Gateway: %s", WiFi.gatewayIP().toString().c_str());
        ESP_LOGI(TAG, "Subnet: %s", WiFi.subnetMask().toString().c_str());
        ESP_LOGI(TAG, "DNS: %s", WiFi.dnsIP().toString().c_str());
        ESP_LOGI(TAG, "RSSI: %d dBm", WiFi.RSSI());
        return true;
    } else {
        ESP_LOGE(TAG, "WiFi connection failed after %d attempts", attempts);
        ESP_LOGE(TAG, "Final WiFi status: %d", WiFi.status());
        
        // Print status explanation
        switch(WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                ESP_LOGE(TAG, "SSID not found - check network name");
                break;
            case WL_CONNECT_FAILED:
                ESP_LOGE(TAG, "Connection failed - check password");
                break;
            case WL_CONNECTION_LOST:
                ESP_LOGE(TAG, "Connection lost");
                break;
            case WL_DISCONNECTED:
                ESP_LOGE(TAG, "Disconnected");
                break;
            default:
                ESP_LOGE(TAG, "Unknown WiFi error");
                break;
        }
        return false;
    }
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::getLocalIP() const {
    return WiFi.localIP().toString();
}

bool NetworkManager::queueBatch(const ReadingBatch& batch) {
    if (!batch_queue) return false;
    
    // Allocate memory for the batch copy
    ReadingBatch* batch_copy = new ReadingBatch(batch);
    
    if (xQueueSend(batch_queue, &batch_copy, 0) == pdTRUE) {
        ESP_LOGI(TAG, "Batch queued for sending (%d readings)", batch.size());
        return true;
    } else {
        delete batch_copy;
        ESP_LOGW(TAG, "Network queue full, dropping batch");
        return false;
    }
}

bool NetworkManager::sendDataBatch(const ReadingBatch& readings) {
    if (!isConnected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot send data");
        return false;
    }

    HTTPClient http;
    if (!http.begin(cfg::SERVER_URL)) {
        ESP_LOGE(TAG, "Failed to begin HTTP connection");
        return false;
    }
    
    http.addHeader("Content-Type", "application/json");

    // Create JSON payload
    JsonDocument doc;
    doc["device_id"] = "ESP32_DHT11";
    doc["batch_time"] = millis();
    doc["readings"].to<JsonArray>();

    for (const auto& reading : readings) {
        JsonObject obj = doc["readings"].add<JsonObject>();
        obj["temperature"] = reading.t;
        obj["humidity"] = reading.h;
        obj["timestamp"] = reading.timestamp;
    }

    String jsonString;
    serializeJson(doc, jsonString);

    ESP_LOGI(TAG, "Sending batch of %d readings", readings.size());
    ESP_LOGD(TAG, "JSON payload: %s", jsonString.c_str());

    int httpResponseCode = http.POST(jsonString);
    
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);
    
    if (success) {
        String response = http.getString();
        ESP_LOGI(TAG, "HTTP Response: %d - %s", httpResponseCode, response.c_str());
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %d", httpResponseCode);
    }

    http.end();
    return success;
}

void NetworkManager::networkTask() {
    ReadingBatch* batch;
    
    for (;;) {
        if (xQueueReceive(batch_queue, &batch, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Processing batch from queue");
            
            bool success = sendDataBatch(*batch);
            if (success) {
                ESP_LOGI(TAG, "Batch sent successfully");
            } else {
                ESP_LOGW(TAG, "Failed to send batch");
                // Could implement retry logic here
            }
            
            delete batch; // Clean up allocated memory
        }
    }
}

void NetworkManager::networkTaskWrapper(void* param) {
    NetworkManager* manager = static_cast<NetworkManager*>(param);
    manager->networkTask();
}
