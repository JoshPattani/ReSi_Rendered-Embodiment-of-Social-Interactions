// Example of arduino secrets file.
// Change the name of your file to 'arduino_secrets.h' and add your network info.

// user identifier (for multi-participant synchronization)
const String USER_A = "A";  // Participant A
const String USER_B = "B";  // Participant B
const String USER = USER_A; // Current participant identifier

#define SECRET_TARGET_IP 127, 0, 0, 1 // Target IP for OSC messages
#define SECRET_TARGET_PORT 12345

// DEFAULT WIFI
#define SECRET_SSID "SSID"
#define SECRET_WIFI_PASSWORD ""

// HOME WIFI
#define SECRET_HOME_SSID "YourWiFiNetwork" // Add your credentials here
#define SECRET_WIFI_PASS "YourWiFiPassword"

// STUDIO WIFI
#define SECRET_STUDIO_SSID "YourStudioWiFiNetwork" // Add your secondary network here
#define SECRET_STUDIO_WIFI_PASS "YourStudioWiFiPassword"

// CAPTIVE PORTAL WIFI
#define SECRET_UCB_SSID "YourCampusWiFiNetwork" // Change to your credentials here
#define SECRET_IDENTIKEY "YourIdentikey"
#define SECRET_PASS "YourPassword"