#!/usr/bin/env python3
import json
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
import datetime
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import numpy as np
from matplotlib.dates import DateFormatter
import matplotlib.dates as mdates

class SensorDataCollector:
    def __init__(self, max_points=100):
        self.max_points = max_points
        self.timestamps = deque(maxlen=max_points)
        self.temperatures = deque(maxlen=max_points)
        self.humidities = deque(maxlen=max_points)
        self.lock = threading.Lock()
        
    def add_readings(self, readings):
        with self.lock:
            for reading in readings:
                # Convert ESP32 timestamp (millis) to datetime
                timestamp = datetime.datetime.now()  # Use current time for better sync
                self.timestamps.append(timestamp)
                self.temperatures.append(reading['temperature'])
                self.humidities.append(reading['humidity'])
                
    def get_data(self):
        with self.lock:
            return (list(self.timestamps), 
                   list(self.temperatures), 
                   list(self.humidities))

# Global data collector
data_collector = SensorDataCollector()

class SensorHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/sensor-data':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                data = json.loads(post_data.decode('utf-8'))
                timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                
                print(f"\n[{timestamp}] Received batch from {data.get('device_id', 'Unknown')}")
                readings = data.get('readings', [])
                print(f"Batch contains {len(readings)} readings:")
                
                for i, reading in enumerate(readings):
                    print(f"  {i+1}: T={reading['temperature']:.1f}°C, H={reading['humidity']:.0f}%, "
                          f"Time={reading['timestamp']}")
                
                # Add readings to collector for plotting
                data_collector.add_readings(readings)
                
                # Save to file (optional)
                with open('sensor_data.jsonl', 'a') as f:
                    f.write(json.dumps(data) + '\n')
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(b'{"status": "success"}')
                
            except json.JSONDecodeError:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b'{"error": "Invalid JSON"}')
        else:
            self.send_response(404)
            self.end_headers()
            
    def log_message(self, format, *args):
        # Suppress HTTP server log messages
        pass

def start_server():
    server = HTTPServer(('0.0.0.0', 8080), SensorHandler)
    print("Server running on http://0.0.0.0:8080")
    print("Waiting for sensor data...")
    server.serve_forever()

def update_plots(frame):
    timestamps, temperatures, humidities = data_collector.get_data()
    
    if len(timestamps) < 2:
        return
    
    # Clear all subplots
    for ax in [ax1, ax2, ax3]:
        ax.clear()
    
    # Temperature vs Time
    ax1.plot(timestamps, temperatures, 'r-', marker='o', markersize=3, linewidth=1.5, label='Temperature')
    ax1.set_ylabel('Temperature (°C)', color='r')
    ax1.tick_params(axis='y', labelcolor='r')
    ax1.grid(True, alpha=0.3)
    ax1.set_title('Temperature vs Time')
    
    if len(timestamps) > 0:
        ax1.xaxis.set_major_formatter(DateFormatter('%H:%M:%S'))
        ax1.tick_params(axis='x', rotation=45)
    
    # Humidity vs Time
    ax2.plot(timestamps, humidities, 'b-', marker='s', markersize=3, linewidth=1.5, label='Humidity')
    ax2.set_ylabel('Humidity (%)', color='b')
    ax2.tick_params(axis='y', labelcolor='b')
    ax2.grid(True, alpha=0.3)
    ax2.set_title('Humidity vs Time')
    
    if len(timestamps) > 0:
        ax2.xaxis.set_major_formatter(DateFormatter('%H:%M:%S'))
        ax2.tick_params(axis='x', rotation=45)
    
    # Temperature vs Humidity (scatter plot)
    if len(temperatures) > 1 and len(humidities) > 1:
        colors = np.linspace(0, 1, len(temperatures))
        scatter = ax3.scatter(humidities, temperatures, c=colors, cmap='viridis', 
                             alpha=0.7, s=30, edgecolors='black', linewidth=0.5)
        
        # Add trend line if we have enough points
        if len(temperatures) > 5:
            z = np.polyfit(humidities, temperatures, 1)
            p = np.poly1d(z)
            ax3.plot(humidities, p(humidities), "r--", alpha=0.8, linewidth=2)
    
    ax3.set_xlabel('Humidity (%)')
    ax3.set_ylabel('Temperature (°C)')
    ax3.grid(True, alpha=0.3)
    ax3.set_title('Temperature vs Humidity Correlation')
    
    # Adjust layout to prevent overlapping
    plt.tight_layout()

if __name__ == '__main__':
    # Set up the plot
    try:
        plt.style.use('seaborn-v0_8')  # Use a nice style
    except OSError:
        # Fallback to basic style if seaborn not available
        plt.style.use('default')
    
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10))
    
    # Start the server in a separate thread
    server_thread = threading.Thread(target=start_server, daemon=True)
    server_thread.start()
    
    # Start the animation
    ani = animation.FuncAnimation(fig, update_plots, interval=1000, cache_frame_data=False)
    
    plt.suptitle('ESP32 Environmental Monitor - Real-time Data', fontsize=16, fontweight='bold')
    
    try:
        plt.show()
    except KeyboardInterrupt:
        print("\nShutting down...")
        plt.close('all')