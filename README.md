# ESP32 Environmental Monitor

A modular ESP32-based environmental monitoring system that measures temperature and humidity using a DHT11 sensor, displays readings on an SSD1306 OLED, and sends data over WiFi to a computer for real-time plotting.

## Hardware Requirements

- ESP32 Development Board
- DHT11 Temperature & Humidity Sensor
- 0.96" I²C SSD1306 OLED Display

## Wiring

```
OLED SDA  → GPIO21
OLED SCL  → GPIO22
OLED VCC  → 3.3V
OLED GND  → GND

DHT11 DATA → GPIO5
DHT11 VCC  → 3.3V
DHT11 GND  → GND
```

## Project Structure

```
src/
├── main.cpp                 # Main application entry point
├── config/
│   └── config.h            # Configuration constants
├── types/
│   └── types.h             # Data structure definitions
├── display/
│   ├── display.h           # Display interface
│   └── display.cpp         # Display implementation
├── sensor/
│   ├── sensor.h            # Sensor interface
│   └── sensor.cpp          # Sensor implementation
├── network/
│   ├── network.h           # Network interface
│   └── network.cpp         # Network implementation
├── tasks/
│   ├── tasks.h             # Task management interface
│   └── tasks.cpp           # Task management implementation
└── listener.py             # Python data receiver with real-time plotting
```

## Features

### ESP32 Side
- **Modular Architecture**: Clean separation of concerns across different modules
- **Non-blocking Network**: Dedicated network task with queue-based batch sending
- **Robust Data Handling**: Sensor data is kept until HTTP POST succeeds
- **Real-time Display**: OLED shows current readings and system status
- **WiFi Status Monitoring**: Visual feedback on connection status

### Python Side
- **Real-time Plotting**: Three live plots showing:
  - Temperature vs Time
  - Humidity vs Time  
  - Temperature vs Humidity correlation
- **Data Persistence**: Automatically saves all received data to `sensor_data.jsonl`
- **Threading**: Non-blocking HTTP server with matplotlib GUI

## Setup Instructions

### 1. ESP32 Configuration

1. Update WiFi credentials in `src/config/config.h`:
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_NAME";
   const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
   ```

2. Update your computer's IP address in `src/config/config.h`:
   ```cpp
   const char* SERVER_URL = "http://YOUR_COMPUTER_IP:8080/sensor-data";
   ```

3. Build and upload using PlatformIO:
   ```bash
   pio run --target upload
   ```

### 2. Python Listener Setup

1. Install required Python packages:
   ```bash
   pip install -r requirements.txt
   ```

2. Run the listener with real-time plotting:
   ```bash
   python src/listener.py
   ```

## Configuration Options

Edit `src/config/config.h` to customize:
- **GPIO pins**: Sensor and I²C pin assignments
- **Timing**: Sensor reading and UI update intervals  
- **Network**: WiFi credentials and server endpoint
- **Data collection**: Readings per batch size

## System Architecture

### Tasks
- **SensorTask**: Reads DHT11 every 2 seconds, buffers readings
- **UiTask**: Updates OLED display every 500ms
- **NetworkTask**: Processes queued data batches for HTTP transmission

### Data Flow
1. Sensor readings are collected and buffered
2. When buffer reaches configured size (10 readings), data is queued for network transmission
3. Network task processes queue asynchronously  
4. Data is only removed from buffer after successful HTTP POST
5. Python listener receives JSON batches and updates real-time plots

## Troubleshooting

- **WiFi connection issues**: Check credentials and network availability
- **Sensor reading errors**: Verify DHT11 wiring and power supply
- **Display not working**: Confirm I²C connections and address (0x3C)
- **No data on computer**: Ensure firewall allows port 8080 and correct IP address is configured

## Data Format

JSON batches sent over HTTP:
```json
{
  "device_id": "ESP32_DHT11",
  "batch_time": 123456789,
  "readings": [
    {
      "temperature": 25.2,
      "humidity": 60,
      "timestamp": 123456789
    }
  ]
}
```
