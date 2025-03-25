# 💧GSR Sensor (Galvanic Skin Response)

## Hardware Requirements

- ESP32 development board (Arduino Nano ESP32 recommended)
- GSR sensor electrodes and analog front-end
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

2. Clone this repository and open the GSR-sensor_Final folder:

   ```bash
   cd path\to\ReSi_Rendered-Embodiment-of-Social-Interactions\MCU_code\PlatformIO\GSR-sensor_Final
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
5. Open the `GSR-sensor_Final.ino` file in Arduino IDE, build and upload

## Hardware Connection

Connect the GSR sensor to your ESP32 board according to the following pinout:

- GSR sensor VCC → 3.3V on ESP32
- GSR sensor GND → GND on ESP32
- GSR sensor SIG → Analog pin on ESP32 (as defined in the code)

## Usage

1. After uploading the firmware, the device will attempt to connect to the configured WiFi network
2. Once connected, it will start reading GSR data and send it via OSC
3. You can monitor the data by connecting to the device's serial port at 115200 baud
4. The device transmits the following OSC messages:
   - `/gsrCurrent` - Raw GSR sensor values

## Electrode Placement

For optimal GSR measurements:

1. Place electrodes on the palmar surface of fingers (index and middle fingers preferred)
2. Ensure good contact between electrodes and skin
3. Minimize movement during measurement to reduce motion artifacts

## Troubleshooting

### WiFi Connection Issues

- Verify your WiFi credentials in `arduino_secrets.h`
- Ensure the WiFi network is in range
- Check if the ESP32 is properly powered

### Poor Signal Quality

- Make sure the electrodes have good skin contact
- Clean the skin with alcohol wipes before attaching electrodes
- Ensure the electrodes are properly moistened if using wet electrodes

### Noisy Data

- Keep the participant still during measurements
- Ensure electrodes are securely attached
- Check for interference from other electronic devices
