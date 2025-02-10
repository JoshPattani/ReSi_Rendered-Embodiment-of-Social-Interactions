#include "thingProperties.h" // Include the generated file

// ----- User Configuration -----
// Replace with your actual Thing ID from Arduino IoT Cloud
// const char *THING_ID = SECRECT_THING_ID;

// // Replace with your WiFi network credentials
// const char *SSID = SECRET_SSID;
// const char *WIFI_PASSWORD = SECRET_PASS;

// Define the analog sensor pin (ESP32 ADC pin, e.g., 34)
const int ANALOG_SENSOR_PIN = A0;

// Variables for heart rate detection
int HRsignal, smoothedSignal;
int lastSignal = 0;
unsigned long lastBeatTime = 0;
unsigned long beatIntervals[3] = {0, 0, 0}; // Store last 3 intervals for small fluctuations
int beatIndex = 0;
int beatsPerMinute = 0;
int beatCount = 0;
unsigned long startTime = 0;
const int minBeatInterval = 300;  // Minimum time between beats (ms)
const int maxBeatInterval = 1500; // Ignore if longer than 1.5 sec (prevents false lows)

// Moving Average Filter (lighter smoothing)
const int filterSize = 4;
int readings[filterSize];
int readIndex = 0;
long total = 0;

// Dynamic Threshold Variables
int peak = 0, trough = 1023;
float thresholdFactor = 0.74; // Adaptive threshold factor
int threshold = 550;          // Default threshold

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

  startTime = millis();

  // Initialize moving average array
  for (int i = 0; i < filterSize; i++)
  {
    readings[i] = analogRead(SENSOR_PIN);
    total += readings[i];
  }

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
  // int reading = analogRead(ANALOG_SENSOR_PIN);

  // Read raw signal
  HRsignal = analogRead(SENSOR_PIN);

  // Light filtering: Moving Average (4 samples)
  total -= readings[readIndex];
  readings[readIndex] = HRsignal;
  total += readings[readIndex];
  readIndex = (readIndex + 1) % filterSize;
  smoothedSignal = total / filterSize;

  // Update peak and trough for dynamic threshold
  if (smoothedSignal > peak)
    peak = smoothedSignal;
  if (smoothedSignal < trough)
    trough = smoothedSignal;
  threshold = trough + (peak - trough) * thresholdFactor;

  // Detect beat using refined peak detection
  unsigned long currentTime = millis();
  if ((smoothedSignal > threshold) && (lastSignal <= threshold) && (currentTime - lastBeatTime) > minBeatInterval)
  {
    unsigned long timeDiff = currentTime - lastBeatTime;

    Serial.println(timeDiff);

    // Ignore false detections (e.g., BPM below 40 or above 180)
    if (timeDiff > minBeatInterval && timeDiff < maxBeatInterval)
    {
      lastBeatTime = currentTime;
      beatCount++;

      // Store the latest interval in a smaller buffer (for finer fluctuations)
      beatIntervals[beatIndex] = timeDiff;
      beatIndex = (beatIndex + 1) % 3; // Keep only the last 3 intervals

      // Compute average BPM using the last 3 intervals (allows small variations)
      unsigned long sumIntervals = 0;
      int validIntervals = 0;
      for (int i = 0; i < 3; i++)
      {
        if (beatIntervals[i] > 0)
        { // Ignore empty slots
          sumIntervals += beatIntervals[i];
          validIntervals++;
        }
      }
      if (validIntervals > 0)
      {
        beatsPerMinute = 60000 / (sumIntervals / validIntervals);
      }

      Serial.println("Beat detected!");

      
    }
  }

  lastSignal = smoothedSignal;

  // Update BPM every 5 seconds (shorter update window for finer changes)
  if (millis() - startTime >= 5000)
  {
    if (beatCount > 0)
    {
      Serial.print("Estimated Heart Rate: ");
      Serial.print(beatsPerMinute);
      Serial.println(" BPM");
    }
    else
    {
      Serial.println("No beats detected.");
    }
    beatCount = 0;
    startTime = millis();
  }

  // Debugging - Print raw and filtered signals
  // Serial.print("Raw Signal: ");
  // Serial.print(signal);
  // Serial.print(" | Filtered: ");
  // Serial.print(smoothedSignal);
  // Serial.print(" | Threshold: ");
  // Serial.print(threshold);
  // Serial.print(" | BPM: ");
  // Serial.println(beatsPerMinute);

  delay(10); // Small delay for stability

  // Publish sensor data
  String payload = String(reading);
  // client.publish(MQTT_TOPIC, payload.c_str());

  // For debugging: print the sensor reading locally.
  Serial.print("Local sensor reading: ");
  Serial.println(payload);

  // Delay between readings (adjust as needed).
  delay(1000);
}
