/*
This file contains the main code for the Atlas project, which is responsible for connecting an Arduino Nano esp32 to the appropriate WiFi network based on the environment, initializing the Arduino IoT Cloud, and sending OSC messages over UDP. It also includes functions for handling network stability and reconnection, as well as callbacks for cloud connection events.

The code begins by including the necessary libraries and header files, such as "networkTypes.h" for defining network types, "customConnectionHandler.h" for custom connection handling, and "thingProperties.h" for defining properties for the Arduino IoT Cloud. It also includes the WiFi and UDP libraries for network communication.

The setup function initializes the serial communication, detects the network type, and connects to the appropriate network based on the type. It then initializes the Arduino IoT Cloud connection and properties, and sets up the UDP connection for local communication. The loop function allows the system to stabilize and periodically checks for network changes.

The main code also includes functions for sending OSC messages, connecting to local ports, and handling cloud connection events. It uses the WiFiStabilityManager library to configure the device for stability and handle network reconnection. The code also includes debug flags and variables for monitoring network status and sensor readings.

Overall, the main code for the Atlas project is well-organized and structured, with clear comments and modular functions for handling network connections, cloud initialization, and communication with external devices.

The current system is supported on the ESP32 platform but can be adapted to other platforms with minimal changes to the network configuration and connection handling functions.

✌️ Cheers!
*/
/*
 * WiFi Network Auto-Selection with OSC Messaging for GSR Sensor
 *
 * This version removes Arduino IoT Cloud connectivity while preserving
 * the ability to automatically detect and connect to different network
 * environments and send OSC messages.
 */
#include <Arduino.h>
#include "networkTypes.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "wifiHandler.h"
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include "wifi_stability.h"

// Network configuration
WiFiUDP Udp;                           // Create a UDP instance for OSC communication
bool UDPConnected = false;             // Flag to check if UDP connection is established
extern NetworkType currentNetworkType; // Reference the type from wifiHandler.h

// IP and port for OSC communication
const IPAddress outIp(SECRET_TARGET_IP); // Use the secret target IP defined in arduino_secrets.h
// Alternatively: const IPAddress outIp;  // Declare first, initialize in setup()
const unsigned int outPort = SECRET_TARGET_PORT; // Target port. Must match the port in your Max/MSP patch.

// Declarations
void sendOSCMessage(const char *address, float value);
void connectLocalPort();

// Debug flag
const bool DEBUG = true;

//   AnalogMode = true -> analog (oscilloscope) mode; false -> digital interval mode.
//   CHANNEL: false selects channel A; true selects channel B.
bool AnalogMode = false; // set to false for digital interval mode (required for HRV)

// --------------------------------------------- //

// =============================== //
// ****** MCU Configuration ****** //
// =============================== //

// Analog pin for PPG sensor input:
const int analogPin = A0; // Analog input pin for heart rate sensor
const int digitalPin = 2; // Digital input pin for heart rate sensor
const int sensorPin = digitalPin;

// --------------------------------------------- //

// =============================== //
// ****** HRV Data Storage ******* //
// =============================== //

// --- HRV data storage (digital mode) ---
// We assume a 2.5‑minute window. At high heart rates, you might see up to 300 beats.
// For simplicity, we use a fixed-size buffer. Modify as needed.
#define HRV_BUFFER_SIZE 300
#define HRV_WINDOW_MS 150000UL // 2.5 minutes in milliseconds

// HR metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
int hRCurrentValue; // Current heart rate
int hRUserMax;      // User-defined maximum heart rate
int hRUserMin;      // User-defined minimum heart rate

// HRV metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
float rmssd; // Root mean square of successive differences
float sdann; // Standard deviation of the average NN intervals
float sdnn;  // Standard deviation of NN intervals

// HRV buffer and index
unsigned long NN_intervals[HRV_BUFFER_SIZE]; // Buffer to store NN intervals
volatile unsigned long NN_index = 0;
volatile unsigned long hrvWindowStart = 0;

// --- Heart Rate Variables ---
unsigned long lastBeatTime = 0;

// Hrv reset function
void resetHRV()
{
  hRCurrentValue = 0;
  hRUserMax = -1000000;
  hRUserMin = 1000000;

  // Reset HRV metrics
  rmssd = 0;
  sdann = 0;
  sdnn = 0;

  for (unsigned long i = 0; i < HRV_BUFFER_SIZE; i++)
  {
    NN_intervals[i] = 0;
  }
  NN_index = 0;
  hrvWindowStart = millis();
}

// --------------------------------------------- //

// =============================== //
// ****** Heart Rate ****** //
// =============================== //
volatile unsigned long lastPulseTime = 0;  // Stores the time of the last beat
volatile unsigned long pulseIntervals[10]; // Stores last 10 pulse intervals
volatile uint8_t pulseIndex = 0;           // Rolling index for interval storage
volatile bool newPulse = false;            // Flag for pulse detection
bool calibrated = false;                   // Flag for user calibration

// Calculate rolling average BPM
int calculateBPM()
{
  unsigned long sum = 0;
  for (int i = 0; i < 10; i++)
  {
    sum += pulseIntervals[i];
  }
  unsigned long avgInterval = sum / 10;

  if (avgInterval > 0)
  {
    return 60000 / avgInterval; // Convert ms to BPM
  }

  return avgInterval;
}

// Interrupt Service Routine (ISR) for detecting heartbeats
void pulseISR()
{
  unsigned long currentTime = millis();
  unsigned long interval = currentTime - lastPulseTime;

  if (interval > 300 && interval < 2000)
  { // Ignore noise (too fast/slow beats)
    pulseIntervals[pulseIndex] = interval;
    pulseIndex = (pulseIndex + 1) % 10; // Move to next slot in rolling average array

    // Store the interval in the NN_intervals buffer for HRV analysis
    // (For simplicity, if the buffer fills, we wrap around.)
    if (NN_index < HRV_BUFFER_SIZE)
    {
      NN_intervals[NN_index++] = interval;
    }
    else
    {
      // Overwrite oldest data (circular buffer)
      NN_intervals[NN_index % HRV_BUFFER_SIZE] = interval;
      NN_index++;
    }

    newPulse = true; // Notify main loop
  }

  lastPulseTime = currentTime;
}

// --------------------------------------------- //

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// OSC message addressing
// "/pulse" -> Pulse detection message
// "/hearRate" -> Heart rate message
// "/rmssd" -> Root mean square of successive differences
// "/sdann" -> Standard deviation of the average NN intervals
// "/sdnn" -> Standard deviation of NN intervals

// Update intervals
const unsigned long oscSendInterval = 500;     // 500ms = 2Hz send rate
const unsigned long cloudSendInterval = 1000;  // 1000ms = 1Hz cloud update rate
const unsigned long minMaxSendInterval = 1000; // 1000ms = 1Hz send rate

// Timers
unsigned long lastOscSendTime = 0;
unsigned long lastCloudSendTime = 0;
unsigned long lastMinMaxSendTime = 0; // timer for min/max updates

void resetTimers()
{
  lastOscSendTime = 0;
  lastCloudSendTime = 0;
  lastMinMaxSendTime = 0;
}

// --------------------------------------------- //

// ============================== //
// ****** HRV Computation ******* //
// ============================== //

// --- HRV Computation ---
// This function computes SDNN, RMSSD, and SDANN from the NN_intervals collected in the current window.
void computeHRVMetrics()
{
  // First, disable interrupts while we copy the buffer.
  noInterrupts();
  int n = NN_index;
  // Create a local copy (in milliseconds) in an array.
  double intervals_ms[HRV_BUFFER_SIZE];
  for (int i = 0; i < n && i < HRV_BUFFER_SIZE; i++)
  {
    intervals_ms[i] = NN_intervals[i];
  }
  // Reset the global buffer for the next window.
  NN_index = 0;
  // Reset the window start time.
  hrvWindowStart = millis();
  interrupts();

  if (n < 2)
  {
    Serial.println("Not enough data for HRV computation.");
    return;
  }

  // Compute the mean of the NN intervals.
  double sum = 0;
  for (int i = 0; i < n; i++)
  {
    sum += intervals_ms[i];
  }
  double mean = sum / n;

  // Compute SDNN: standard deviation of all NN intervals.
  double sdnn_sum = 0;
  for (int i = 0; i < n; i++)
  {
    double diff = intervals_ms[i] - mean;
    sdnn_sum += diff * diff;
  }
  double sdnn = sqrt(sdnn_sum / n);

  // Compute RMSSD: square root of the mean squared differences of successive NN intervals.
  double rmssd_sum = 0;
  int count_diff = 0;
  for (int i = 0; i < n - 1; i++)
  {
    double diff = intervals_ms[i + 1] - intervals_ms[i];
    rmssd_sum += diff * diff;
    count_diff++;
  }
  double rmssd = (count_diff > 0) ? sqrt(rmssd_sum / count_diff) : 0;

  // Compute SDANN: approximate by dividing the window into 5 equal segments.
  int segments = 5;
  if (n < segments)
  {
    Serial.println("Not enough data for SDANN computation.");
    return;
  }
  int segSize = n / segments;
  double segMeans[5];
  for (int s = 0; s < segments; s++)
  {
    double segSum = 0;
    for (int i = s * segSize; i < (s + 1) * segSize; i++)
    {
      segSum += intervals_ms[i];
    }
    segMeans[s] = segSum / segSize;
  }
  // Compute standard deviation of the segment means.
  double segMeanSum = 0;
  for (int s = 0; s < segments; s++)
  {
    segMeanSum += segMeans[s];
  }
  double segMeanAvg = segMeanSum / segments;
  double sdann_sum = 0;
  for (int s = 0; s < segments; s++)
  {
    double diff = segMeans[s] - segMeanAvg;
    sdann_sum += diff * diff;
  }
  double sdann = sqrt(sdann_sum / segments);

  if (DEBUG)
  {
    // Print the computed HRV metrics.
    Serial.println();
    Serial.print("HRV Metrics: SDNN = ");
    Serial.print(sdnn);
    Serial.print(" ms, RMSSD = ");
    Serial.print(rmssd);
    Serial.print(" ms, SDANN = ");
    Serial.print(sdann);
    Serial.println(" ms");
    Serial.println();
  }
}

// --------------------------------------------- //

void setup()
{
  // Initialize serial and wait for port to open
  Serial.begin(9600);
  delay(4000); // Give more time for serial to initialize

  Serial.println("Starting RESI WiFi detection and connection (No Cloud Version)...");

  // Configure device for stability
  WiFiStabilityManager::configureDevice();

  // First detect network type
  currentNetworkType = detectNetworkType();

  // Connect to the appropriate network based on type
  bool wifiConnected = false;

  if (currentNetworkType == NETWORK_HOME)
  {
    Serial.println("Home network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }
  else if (currentNetworkType == NETWORK_STUDIO)
  {
    Serial.println("Studio network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
  }
  else if (currentNetworkType == NETWORK_CAMPUS)
  {
    Serial.println("Campus network detected");
    // Try campus connection (this will handle authentication too)
    if (handleUCBWirelessAuth())
    {
      Serial.println("Successfully connected to campus network");
      wifiConnected = true;

      // Ensure we're stable before proceeding
      WiFiStabilityManager::waitForStableConnection();
    }
    else
    {
      Serial.println("Failed to connect to campus network");
    }
  }
  else
  {
    Serial.println("Unknown network environment");
    // Try home network as default
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }

  // Check final connection status
  Serial.print("Final WiFi status: ");
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Initialize UDP for OSC communications
  if (WiFi.status() == WL_CONNECTED)
  {
    connectLocalPort();
  }

  // Initialize PPG sensor
  // For HRV, we use the digital pin for interval measurement.
  if (!AnalogMode)
  {
    pinMode(sensorPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulseISR, RISING);
    // Initialize the last beat time.
    lastBeatTime = millis();
    // Set the HRV window start time.
    hrvWindowStart = millis(); // Change from micros() to millis() for consistency

    resetTimers();

    // Initialize HRV metrics.
    resetHRV();

    // Initialize array with reasonable values (assume ~60 BPM at start)
    for (int i = 0; i < 10; i++)
    {
      pulseIntervals[i] = 1000;
    }
  }
  else
  {
    // Analog mode: set up analog pin
    pinMode(analogPin, INPUT);
  }

  // Check final connection status
  Serial.print("Final WiFi status: ");
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to: ");
    Serial.println(WiFi.SSID());
  }
}

void loop()
{
  // Allow system to stabilize and prevent watchdog resets
  esp_task_wdt_reset();

  static unsigned long lastNetworkCheck = 0;
  static String lastSSID = WiFi.SSID();
  String currentSSID = WiFi.SSID();

  // Only check network periodically to reduce overhead
  if (millis() - lastNetworkCheck > 10000)
  { // Every 10 seconds
    lastNetworkCheck = millis();

    // Monitor for network changes less frequently
    if (lastSSID != currentSSID)
    {
      Serial.print("Network change detected: ");
      Serial.print(lastSSID);
      Serial.print(" -> ");
      Serial.println(currentSSID);
      lastSSID = currentSSID;
    }

    // If WiFi is disconnected, try to reconnect without heavy operations
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi disconnected, attempting to reconnect");

      // Different approach based on network type
      if (currentNetworkType == NETWORK_CAMPUS)
      {
        WiFi.begin(SECRET_UCB_SSID);
      }
      else if (currentNetworkType == NETWORK_STUDIO)
      {
        WiFi.begin(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
      }
      else
      {
        WiFi.begin(SECRET_HOME_SSID, SECRET_WIFI_PASS);
      }

      // Check if we need to reinitialize UDP
      if (WiFi.status() == WL_CONNECTED && !UDPConnected)
      {
        connectLocalPort();
      }
    }
  }

  unsigned long currentTime = millis();
  // Send OSC messages
  if (WiFi.status() == WL_CONNECTED && UDPConnected)
  {

    // Beat intervals are captured in the ISR.
    // Check if the current window (2.5 minutes) has elapsed.
    unsigned long now = millis(); // Changed from mis() to millis() for consistency
    if (!AnalogMode && (now - hrvWindowStart) >= HRV_WINDOW_MS)
    {
      // Compute and output HRV metrics for the current window.
      computeHRVMetrics();
      // (The computeHRVMetrics() function resets the window start and clears the buffer.)

      // Send the HRV metrics through OSC.
      String addressRMSSD = "/User_" + String(USER) + "/hrv/rmssd";
      sendOSCMessage(addressRMSSD.c_str(), rmssd);

      String addressSDANN = "/User_" + String(USER) + "/hrv/sdann";
      sendOSCMessage(addressSDANN.c_str(), sdann);

      String addressSDNN = "/User_" + String(USER) + "/hrv/sdnn";
      sendOSCMessage(addressSDNN.c_str(), sdnn);
    }
    if (newPulse)
    {
      newPulse = false;

      // Sending pulse message with static value indicating pulse detection
      String address = "/User_" + String(USER) + "/pulse";
      sendOSCMessage(address.c_str(), 1);

      // Update the cloud variable with the latest BPM value.
      hRCurrentValue = calculateBPM(); // Assuming calculateBPM() is a function that computes the latest BPM

      if (hRCurrentValue > 0)
      {
        if (DEBUG)
        {
          Serial.println();
          Serial.print("Pulse detected at: ");
          Serial.print(currentTime);
          Serial.print(" with interval: ");
          Serial.print(pulseIntervals[pulseIndex]);
          Serial.print(" ms, BPM: ");
          Serial.println(hRCurrentValue);
          Serial.println();
        }

        // Send OSC messages at specified interval
        if (currentTime - lastOscSendTime >= oscSendInterval)
        {
          lastOscSendTime = currentTime;
          String address = "/User_" + String(USER) + "/hr/value";
          sendOSCMessage(address.c_str(), hRCurrentValue);
        }

        if (!calibrated)
        {
          hRUserMin = hRCurrentValue;
          hRUserMax = hRCurrentValue;
          calibrated = true;
          String addressMin = "/User_" + String(USER) + "/hr/min";
          sendOSCMessage(addressMin.c_str(), hRUserMin);
          String addressMax = "/User_" + String(USER) + "/hr/max";
          sendOSCMessage(addressMax.c_str(), hRUserMax);
        }
        else
        {
          if (hRUserMax < hRCurrentValue)
          {
            hRUserMax = hRCurrentValue;
            String addressMax = "/User_" + String(USER) + "/hr/max";
            sendOSCMessage(addressMax.c_str(), hRUserMax);
          }
          if (hRUserMin > hRCurrentValue)
          {
            hRUserMin = hRCurrentValue;
            String addressMin = "/User_" + String(USER) + "/hr/min";
            sendOSCMessage(addressMin.c_str(), hRUserMin);
          }
        }
      }
      // Send pulse off message
      sendOSCMessage(address.c_str(), 0);
    }
  }
}

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// Connect to local UDP port for OSC communication
void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  Udp.begin(outPort); // Local port to listen on (doesn't matter much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Begin UDP packet
  Udp.beginPacket(outIp, outPort);

  // Write OSC message to UDP
  msg.send(Udp);

  // End packet and send
  Udp.endPacket();

  // Free space
  msg.empty();

  if (DEBUG)
  {
    Serial.print("Sent OSC message: ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(value);
    Serial.print("...to IP address: ");
    Serial.print(outIp);
    Serial.print(" on port: ");
    Serial.println(outPort);
  }
}