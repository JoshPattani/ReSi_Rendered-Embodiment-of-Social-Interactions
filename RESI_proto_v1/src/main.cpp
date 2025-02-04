#include "thingProperties.h" // Include the generated file

// ----- User Configuration -----
// Replace with your actual Thing ID from Arduino IoT Cloud
// const char *THING_ID = SECRECT_THING_ID;

// // Replace with your WiFi network credentials
// const char *SSID = SECRET_SSID;
// const char *WIFI_PASSWORD = SECRET_PASS;

// Define the analog sensor pin (ESP32 ADC pin, e.g., 34)
const int ANALOG_SENSOR_PIN = A0;

// ----- Cloud Variable -----
// This variable will be sent to the Arduino IoT Cloud and can be later retrieved via the API.
// CloudVariable sensorValue;

// ----- WiFi Connection Handler -----
// This object handles the WiFi connection used by the Arduino IoT Cloud.
// WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, WIFI_PASSWORD);

// ----- Callback Function -----
// This function is called whenever the cloud value is updated (for example, if you monitor changes on the dashboard).
// void onSensorValueChange()
// {
//   Serial.print("Cloud variable sensorValue updated: ");
//   Serial.println(sensorValue);
// }

// ----- Initialize Cloud Properties -----
// You must register each cloud property so that the IoT Cloud knows about it.
// void initProperties()
// {
//   // Set the Thing ID for this device
//   ArduinoIoTCloud.setThingId(THING_ID);

//   // Register sensorValue as a READ-only property that triggers on changes.
//   ArduinoIoTCloud.addProperty(sensorValue, READ, ON_CHANGE, onSensorValueChange);
// }

void setup()
{
  // Initialize serial communication for debugging.
  Serial.begin(115200);
  delay(1500); // Give time for the serial monitor to initialize

  // Initialize cloud properties (register variables, etc.)
  initProperties();

  // Begin the connection to the Arduino IoT Cloud.
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Optional: Set the debug message level (0 = off, 2 = verbose)
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Configure MQTT
  // client.setServer(MQTT_SERVER, MQTT_PORT);
  // client.setCallback(callback);
}

void loop()
{
  // Keep the connection with Arduino IoT Cloud alive and process incoming events.
  ArduinoCloud.update();

  // Ensure the connection to the MQTT server is maintained
  // if (!client.connected())
  // {
  //   connectToMqtt();
  // }
  // client.loop();

  // Read the analog sensor value.
  int reading = analogRead(ANALOG_SENSOR_PIN);

  // Publish sensor data
  String payload = String(reading);
  // client.publish(MQTT_TOPIC, payload.c_str());

  // For debugging: print the sensor reading locally.
  Serial.print("Local sensor reading: ");
  Serial.println(payload);

  // Delay between readings (adjust as needed).
  delay(1000);
}
