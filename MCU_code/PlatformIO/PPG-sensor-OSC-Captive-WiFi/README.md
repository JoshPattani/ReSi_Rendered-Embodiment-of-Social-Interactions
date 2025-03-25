# 💓 PPG Sensor (Pulse/Heart Rate)

This ESP32-based photoplethysmography (PPG) sensor measures heart rate and transmits the data over WiFi using OSC messages.

## Hardware Requirements

- ESP32 development board (Arduino Nano ESP32 recommended)
- Pulse sensor (e.g., MAX30102 or compatible)
- USB cable for programming
- Power source (USB or battery)

## Software Requirements

- PlatformIO IDE (recommended) or Arduino IDE
- Required libraries (automatically installed by PlatformIO):
  - WiFi
  - OSC
  - Sensor-specific libraries

## Setup Instructions

### Using PlatformIO (Recommended)

1. Install PlatformIO in VS Code: https://platformio.org/install/ide?install=vscode

2. Clone this repository and open the PPG-sensor_Final folder:

   ```bash
   cd path\to\ReSi_Rendered-Embodiment-of-Social-Interactions\MCU_code\PlatformIO\PPG-sensor_Final
   ```

3. Create a file called arduino_secrets.h in the include directory with your WiFi credentials:

   ```cpp
   #define SECRET_SSID "YourWiFiNetworkName"
   #define SECRET_PASS "YourWiFiPassword"
   #define SECRET_STUDI0_SSID "YourWiFiNetworkName"
   #define SECRET_STUDIO_PASS "YourWiFiPassword"
   #define OSC_DESTINATION_IP "192.168.1.100" // IP address of the computer receiving OSC
   #define OSC_PORT 8000 // Port for OSC messages
   ```

4. Build and upload the firmware:
   - Connect your ESP32 to your computer
   - In VS Code with PlatformIO, click the Upload button (right arrow icon)
   - Or use the PlatformIO CLI:
     '''bash
     pio run -t upload
     '''

### Using Arduino IDE

1. Install the [Arduino IDE](https://www.arduino.cc/en/software)

2. Add ESP32 board support to Arduino IDE:

   - In Arduino IDE, go to File > Preferences
   - Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to Additional Board Manager URLs
   - Go to Tools > Board > Boards Manager
   - Search for ESP32 and install

3. Install required libraries:
   - Go to Tools > Manage Libraries
   - Search for and install: WiFi, OSC, and any sensor-specific libraries
4. Create the `arduino_secrets.h` file as described above
5. Open the `PPG-sensor_Final.ino` file in Arduino IDE, build and upload

## Hardware Connection

Connect the PPG sensor to your ESP32 board according to the following pinout:

    * PPG sensor VCC → 3.3V on ESP32
    * PPG sensor GND → GND on ESP32
    * PPG sensor SIG → Digital pin 2 on ESP32 (or as defined in the code)

## Usage

1. After uploading the firmware, the device will attempt to connect to the configured WiFi network
2. Once connected, it will start reading PPG data and send it via OSC
3. You can monitor the data by connecting to the device's serial port at 115200 baud
4. The device transmits the following OSC messages:
   - `/ppg/hrCurrent` - Current heart rate in BPM
   - `/ppg/hrv` - Heart rate variability data

## Troubleshooting

### WiFi Connection Issues

- Verify your WiFi credentials in `arduino_secrets.h`
- Ensure the WiFi network is in range
- Check if the ESP32 is properly powered

### Sensor Not Working

- Verify connections between the sensor and ESP32
- Make sure the sensor is properly placed on the skin
- Check sensor orientation (some sensors must face a specific direction)

### No OSC Data Received

- Verify the OSC destination IP and port
- Ensure your computer is on the same network as the ESP32
- Check that no firewall is blocking UDP communication
