/*
📡 RESI: Remote Environmental Sensing - PPG sensor

This code snippet is part of the RESI project, which aims to provide a platform for remote environmental sensing using physiological sensors. This snippet is for the PPG sensor, which measures heart rate and heart rate variability (HRV) using a photoplethysmography sensor.

The current system is supported on the ESP32 platform but can be adapted to other platforms with minimal changes to the network configuration and connection handling functions.

✌️ Cheers!
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

// Multiple IP and port targets for OSC communication
const IPAddress outIpAudio(SECRET_AUDIO_IP);           // Audio computer. Defined in arduino_secrets.h
const unsigned int outPortAudio = SECRET_AUDIO_PORT;   // Target port. Must match the port in Max
const IPAddress outIpVisual(SECRET_VISUAL_IP);         // Visual computer
const unsigned int outPortVisual = SECRET_VISUAL_PORT; // Target port. Must match the port in Max

// Function prototypes
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
#define HRV_BUFFER_SIZE 300         // Buffer size for NN intervals
#define HRV_WINDOW_MS 150000UL      // 2.5 minutes in milliseconds
#define HRV_UPDATE_INTERVAL 30000UL // Update every 30 seconds

// Add these variables for sliding window management
unsigned long lastHrvUpdateTime = 0;
bool initialHrvWindowFilled = false;

// HR metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
unsigned long hRcurrent_millis; // Current time in milliseconds
int hRCurrentValue;             // Current heart rate
int hRUserMax;                  // User-defined maximum heart rate
int hRUserMin;                  // User-defined minimum heart rate

// HRV metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
float rmssd; // Root mean square of successive differences
float rmssdMin;
float rmssdMax;
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
  hRcurrent_millis = 0;
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
bool HR_calibrated = false;                // Flag for user heart rate calibration
bool HRV_calibrated = false;               // Flag for HRV calibration

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
  else
  {
    return 0;
  }
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
    hRcurrent_millis = interval;

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
const unsigned long oscSendInterval = 600;     // 500ms = 2Hz send rate
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
void computeHRVMetricsSliding()
{
  // First, disable interrupts while we work with the buffer
  noInterrupts();

  // Determine how many intervals to use (up to the buffer size)
  int n = min((int)NN_index, HRV_BUFFER_SIZE);

  // Create a local copy of intervals for calculations
  double intervals_ms[HRV_BUFFER_SIZE];
  for (int i = 0; i < n; i++)
  {
    intervals_ms[i] = NN_intervals[i];
  }

  // If we have more intervals than the buffer size, we need to manage them
  // This handles the case where NN_index > HRV_BUFFER_SIZE (cyclic buffer)
  if (NN_index >= HRV_BUFFER_SIZE)
  {
    // Shift buffer to make room for new values but keep the window
    // We don't reset NN_index in the sliding approach

    // Only shift if we're at the update interval (not the first calculation)
    if (initialHrvWindowFilled)
    {
      // Number of new elements since last update (approximately)
      int newElements = HRV_UPDATE_INTERVAL / 800; // Rough estimate based on average HR

      // Shift elements to remove oldest ones while keeping the window size
      for (int i = 0; i < HRV_BUFFER_SIZE - newElements; i++)
      {
        NN_intervals[i] = NN_intervals[i + newElements];
      }

      // Adjust the index to account for the shift
      NN_index = HRV_BUFFER_SIZE - newElements;
    }
  }

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
  sdnn = sqrt(sdnn_sum / n);

  // Compute RMSSD: square root of the mean squared differences of successive NN intervals.
  double rmssd_sum = 0;
  int count_diff = 0;
  for (int i = 0; i < n - 1; i++)
  {
    double diff = intervals_ms[i + 1] - intervals_ms[i];
    rmssd_sum += diff * diff;
    count_diff++;
  }
  rmssd = (count_diff > 0) ? sqrt(rmssd_sum / count_diff) : 0;

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
  sdann = sqrt(sdann_sum / segments);

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
    unsigned long now = millis();
    if (!AnalogMode &&
        ((!initialHrvWindowFilled && (now - hrvWindowStart) >= HRV_WINDOW_MS) ||        // Initial window filled
         (initialHrvWindowFilled && (now - lastHrvUpdateTime) >= HRV_UPDATE_INTERVAL))) // Update interval reached
    {
      // Compute HRV metrics without clearing the entire buffer
      computeHRVMetricsSliding();

      // If this is the first calculation, mark the initial window as filled
      if (!initialHrvWindowFilled)
      {
        initialHrvWindowFilled = true;
      }

      // Update last calculation time
      lastHrvUpdateTime = now;

      // Send HRV metrics via OSC
      String addressRMSSD = "/User_" + String(USER) + "/hrv/rmssd";
      sendOSCMessage(addressRMSSD.c_str(), rmssd);

      String addressSDANN = "/User_" + String(USER) + "/hrv/sdann";
      sendOSCMessage(addressSDANN.c_str(), sdann);

      String addressSDNN = "/User_" + String(USER) + "/hrv/sdnn";
      sendOSCMessage(addressSDNN.c_str(), sdnn);
      // Update Min/Max values for RMSSD
      if (!HRV_calibrated)
      {
        rmssdMin = rmssd;
        rmssdMax = rmssd;
        String addressMin = "/User_" + String(USER) + "/hrv/rmssdMin";
        sendOSCMessage(addressMin.c_str(), rmssdMin);
        String addressMax = "/User_" + String(USER) + "/hrv/rmssdMax";
        sendOSCMessage(addressMax.c_str(), rmssdMax);

        HRV_calibrated = true;
      }
      else
      {
        if (rmssdMax < rmssd)
        {
          rmssdMax = rmssd;
          String addressMax = "/User_" + String(USER) + "/hrv/rmssdMax";
          sendOSCMessage(addressMax.c_str(), rmssdMax);
        }
        if (rmssdMin > rmssd)
        {
          rmssdMin = rmssd;
          String addressMin = "/User_" + String(USER) + "/hrv/rmssdMin";
          sendOSCMessage(addressMin.c_str(), rmssdMin);
        }
      }
    }
    if (newPulse)
    {
      newPulse = false;

      // Sending pulse message with static value indicating pulse detection
      String address = "/User_" + String(USER) + "/pulse";
      sendOSCMessage(address.c_str(), 1);

      // Update OSC variable with the latest BPM value.
      hRCurrentValue = calculateBPM(); // computes the latest BPM

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

          String addressInterval = "/User_" + String(USER) + "/hr/interval";
          sendOSCMessage(addressInterval.c_str(), hRcurrent_millis);
        }

        if (!HR_calibrated)
        {
          hRUserMin = hRCurrentValue;
          hRUserMax = hRCurrentValue;
          HR_calibrated = true;
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
  Udp.begin(outPortVisual); // Local port to listen on (doesn't matter much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Send to audio computer
  Udp.beginPacket(outIpAudio, outPortAudio);
  msg.send(Udp);
  Udp.endPacket();

  // Send to visual computer
  Udp.beginPacket(outIpVisual, outPortVisual);
  msg.send(Udp);
  Udp.endPacket();

  // Free space
  msg.empty();

  // Debug message but skip if address string contains "/pulse"
  if (DEBUG && !strstr(address, "/pulse"))
  {
    Serial.print("Sent OSC message: ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(value);
    Serial.print("...to both IP addresses: ");
    Serial.print(outIpAudio);
    Serial.print("(audio) and ");
    Serial.print(outIpVisual);
    Serial.println("(visual)");
  }
}