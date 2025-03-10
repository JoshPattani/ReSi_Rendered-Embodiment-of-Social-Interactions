/*
This example will show you how to configure your device to the ArduinoIoTCloud library and send data to an OSC server.

1. Start by changing the name of this file to `arduino_secrets.h` and fill in the information below.

2. Add the `arduino_secrets.h` file to your `.gitignore` file to keep your personal information safe.



Run in the terminal: `IPconfig/all` to find your IP address (IPv4).
*/

// =============================== //
// *********** WiFi ************** //
// =============================== //

/*
IP ADDRESSING
- IP addresses are used to identify devices on a network

HOME NETWORK
- IP Address: your_ip_address_here. Local IP should work too (127, 0, 0, 1)

Black Box STUDIO NETWORK
- Your Computer IP Address: your_ip_address_here
- Bbox Mac IP Address: 192, 168, 0, 10
- Bbox Mac 2 IP Address: 192, 168, 0, 11

UCB NETWORK
- Your Computer IP Address: your_ip_address_here
*/

#define SECRET_TARGET_IP 127, 0, 0, 1 // () Home, () UCB, () B2
#define SECRET_TARGET_PORT 42069

// DEFAULT WIFI
#define SECRET_SSID ""
#define SECRET_WIFI_PASSWORD ""

// HOME WIFI
#define SECRET_HOME_SSID "Your_home_wifi_ssid_here"
#define SECRET_WIFI_PASS "your_home_wifi_password_here"

// b2 Studio WIFI - Black Box Performance Studio (Shouldn't need to change)
#define SECRET_STUDIO_SSID "BlackBox_wifi_ssid_here"             // Corrected definition name
#define SECRET_STUDIO_WIFI_PASS "your_studio_wifi_password_here" // Updated WIFI password for studio

// Captive Portal
#define SECRET_UCB_SSID "UCB Wireless"
#define SECRET_IDENTIKEY "your_identify_key_here"
#define SECRET_PASS "your_password_here"