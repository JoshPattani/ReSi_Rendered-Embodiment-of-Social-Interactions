// Only set WiFi mode on ESP8266 (ESP32 already configures WiFi appropriately)
#if defined(ARDUINO_ARCH_ESP8266)
WiFi.mode(WIFI_STA);
#endif