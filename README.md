# IntelliCollar: Smart Animal Based Health Collar

## Overview
This project is an **ESP32-based animal health tracking system** that monitors vital parameters such as **location, activity, and heart rate**. It utilizes GPS, an MPU6050 motion sensor, and a heart rate sensor to provide real-time data, which can be accessed through a web interface.

## Features
- **GPS Tracking:** Monitors the animal's location in real-time.
- **Activity Monitoring:** Uses MPU6050 to track movements and detect anomalies.
- **Heart Rate Monitoring:** Measures the animal's heart rate for health assessment.
- **Web Interface:** Displays collected data for easy access and analysis.

## Hardware Components
- **ESP32** (Microcontroller)
- **GPS Module** (for location tracking)
- **MPU6050** (for motion and activity tracking)
- **Heart Rate Sensor** (for health monitoring)
- **Battery & Power Management Circuit**

## Software & Libraries
- **Arduino IDE** (for programming the ESP32)
- **ESPAsyncWebServer** (for hosting the web interface)
- **TinyGPS++** (for parsing GPS data)
- **Wire.h** (for I2C communication with MPU6050)
- **PulseSensorPlayground** (for heart rate monitoring)

## Installation & Setup
1. **Clone the repository:**
   ```sh
   git clone https://github.com/VeralGupta/IntelliCollar.git
   cd IntelliCollar
   ```
2. **Install dependencies:**
   - Open the project in **Arduino IDE**
   - Install required libraries using **Library Manager**
3. **Flash the code to ESP32:**
   - Select the appropriate board (**ESP32 Dev Module**) and COM port
   - Compile and upload the code
4. **Power the ESP32 and monitor data via the web interface**

## Usage
- Connect to the ESP32's **WiFi Access Point** (or configure it for your network)
- Open a web browser and navigate to `http://<ESP32-IP>`
- View real-time GPS location, activity data, and heart rate

## Contributing
Contributions are welcome! Feel free to fork this repository, submit issues, or make pull requests.

## License
This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**. See the [LICENSE](LICENSE) file for details.

