/*
This example will show you how to configure your device to the ArduinoIoTCloud library and send data to an OSC server.

Start by changing the name of this file to `arduino_secrets.h` and fill in the information below.

Be sure to add the `arduino_secrets.h` file to your `.gitignore` file to keep your personal information safe.

This file allows for easier hot-swapping of devices and WiFi networks by defining the necessary information in one place. Uncomment the device(s) and WiFi you are using and fill in the necessary information.
*/
// =============================== //
// *********** Devices *********** //
// =============================== //
// RESI_1
#define SECRET_DEVICE "Your_Device_1_ID"
#define SECRET_DEVICE_KEY "Your_Device_1_Key"

// RESI_2
// #define SECRET_DEVICE "Your_Device_2_ID"
// #define SECRET_DEVICE_KEY "Your_Device_2_Key"

// RESI_3
// #define SECRET_DEVICE "Your_Device_3_ID"
// #define SECRET_DEVICE_KEY "Your_Device_3_Key"

// RESI_4
// #define SECRET_DEVICE "Your_Device_4_ID"
// #define SECRET_DEVICE_KEY "Your_Device_4_Key"

// Add more devices here as needed
// #define SECRET_DEVICE_ID "Your_Device_ID"
// #define SECRET_DEVICE_KEY "Your_Device"

// =============================== //
// *********** WiFi ************** //
// =============================== //

// IP Address and port for OSC communication
#define SECRET_TARGET_IP 192, 168, 0, 8
#define SECRET_TARGET_PORT 42069

// Your Home WiFi
#define SECRET_SSID "Your_SSID"
#define SECRET_WIFI_PASS "Your_Password"

// Studio WiFi
// #define SECRET_SSID "Your_Studio"
// #define SECRET_WIFI_PASS "Your_Password"

// Captive Portal
#define SECRET_UCB_SSID "UCB Wireless"
#define SECRET_IDENTIKEY "your_identify_key_here"
#define SECRET_PASS "your_password_here"