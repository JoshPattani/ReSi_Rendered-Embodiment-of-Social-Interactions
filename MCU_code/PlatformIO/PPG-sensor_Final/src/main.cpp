/*
📡 RESI: Remote Environmental Sensing - PPG sensor

This script is part of the RESI project, which aims to provide a platform for remote environmental sensing using physiological sensors. This snippet is for the PPG sensor, which measures heart rate and heart rate variability (HRV) using a photoplethysmography sensor.

The current system is supported on the ESP32 platform but can be adapted to other platforms with minimal changes to the network configuration and connection handling functions.

EDIT: I have added a new function to handle the discovery of the audio and visual computers. This function will send a UDP broadcast message to the network, and the computers will respond with their IP addresses. The ESP32 will then store these IP addresses for later use in OSC communication.
The function is called `listenForDiscovery()` and is called in the `setup()` function. The IP addresses are stored in the `discoveredAudioIP` and `discoveredVisualIP` variables.

Author: Josh Pattani, 2025
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

// =============================== //
// ****** MCU Configuration ****** //
// =============================== //

// Debug flag
// Set to false for production mode (no serial, sends OSC debug messages instead)
const bool DEBUG = true; // set to true for debugging messages in the serial monitor

// AnalogMode = true -> analog (oscilloscope) mode; false -> digital interval mode.
bool AnalogMode = false; // set to false for digital interval mode (required for HRV)

// Analog pin for PPG sensor input:
const int analogPin = A0; // Analog input pin for heart rate sensor
const int digitalPin = 2; // Digital input pin for heart rate sensor
const int sensorPin = digitalPin;

String user = "/User_" + String(USER); // User identifier (A or B)

void printMemInfo()
{
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
}
// --------------------------------------------- //

// =============================== //
// **** Network Configuration **** //
// =============================== //

// Network configuration
WiFiUDP Udp;                           // Create a UDP instance for OSC communication
bool UDPConnected = false;             // Flag to check if UDP connection is established
extern NetworkType currentNetworkType; // Reference the type from wifiHandler.h

// IP address variables
IPAddress discoveredAudioIP;  // Will store dynamically discovered audio computer IP
IPAddress discoveredVisualIP; // Will store dynamically discovered visual computer IP
bool audioIPDiscovered = false;
bool visualIPDiscovered = false;

// Multiple IP and port targets for OSC communication
const unsigned int inPort = SECRET_LOCAL_PORT;         // Port to listen for OSC messages
const IPAddress outIpAudio(SECRET_AUDIO_IP);           // Audio computer. Defined in arduino_secrets.h
const unsigned int outPortAudio = SECRET_AUDIO_PORT;   // Target port. Must match the port in Max
const IPAddress outIpVisual(SECRET_VISUAL_IP);         // Visual computer
const unsigned int outPortVisual = SECRET_VISUAL_PORT; // Target port. Must match the port in Max
// --------------------------------------------- //

// =============================== //
// ***** Function Prototypes ***** //
// =============================== //

// Function prototypes
void listenForDiscovery();
void sendOSCMessage(const char *address, float value);
void connectLocalPort();
void sendAcknowledgment(IPAddress targetIP, unsigned int targetPort, const char *type, const char *user);
void resetHRV();
// --------------------------------------------- //

// =============================== //
// ****** HRV Data Storage ******* //
// =============================== //

// --- HRV data storage (digital mode) ---
// We assume a 2‑minute window. At high heart rates, you might see up to 240 beats.
// For simplicity, we use a fixed-size buffer. Modify as needed.
#define HRV_BUFFER_SIZE 500         // Buffer size for NN intervals
#define HRV_WINDOW_MS 120000UL      // 2 minutes in milliseconds
#define HRV_UPDATE_INTERVAL 10000UL // Update every 10 seconds

// Variables for sliding window management
unsigned long lastHrvUpdateTime = 0;
bool initialHrvWindowFilled = false;
bool HRV_calibrated = false; // Flag for HRV calibration

// Gaussian filter parameters
const int kernelSize = 3;
const double sigma = 0.7;

// HRV metrics
float rmssd; // Root mean square of successive differences
float rmssdMin;
float rmssdMax;
float sdann; // Standard deviation of the average NN intervals
float sdnn;  // Standard deviation of NN intervals

// HRV buffer and index
unsigned long NN_intervals[HRV_BUFFER_SIZE]; // Buffer to store NN intervals
volatile unsigned long NN_index = 0;
volatile unsigned long hrvWindowStart = 0;

// Global arrays to replace local ones in computeHRVMetricsSliding
double g_intervals_ms[HRV_BUFFER_SIZE];
double g_cleaned_intervals[HRV_BUFFER_SIZE];
double g_filtered_intervals[HRV_BUFFER_SIZE];
// --------------------------------------------- //

// ============================== //
// ********* Heart Rate ********* //
// ============================== //

// --- Heart Rate Variables ---
unsigned long lastBeatTime = 0;
volatile unsigned long lastPulseTime = 0;  // Stores the time of the last beat
volatile unsigned long pulseIntervals[10]; // Stores last 10 pulse intervals
volatile uint8_t pulseIndex = 0;           // Rolling index for interval storage
volatile bool newPulse = false;            // Flag for pulse detection

// HR metrics
unsigned long hRcurrent_millis; // Current time in milliseconds
int hRCurrentValue;             // Current heart rate
int hRUserMax;                  // User-defined maximum heart rate
int hRUserMin;                  // User-defined minimum heart rate
bool HR_calibrated = false;     // Flag for user heart rate calibration

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

// Send acknowledgment back to the discovered computer
void sendAcknowledgment(IPAddress targetIP, unsigned int targetPort, const char *type, const char *user)
{
  // small delay to ensure the packet is sent correctly
  delay(10);

  OSCMessage msg("/arduino/acknowledgment");
  msg.add(type);
  msg.add(user);

  Udp.beginPacket(targetIP, targetPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  // Add a second acknowledgment for redundancy
  delay(100);
  Udp.beginPacket(targetIP, targetPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  if (DEBUG)
  {
    Serial.print("Sent acknowledgment to ");
    Serial.print(type);
    Serial.print(" computer at IP: ");
    Serial.println(targetIP);
  }
}

void listenForDiscovery()
{
  // Check if discovery messages are available
  int packetSize = Udp.parsePacket();

  if (packetSize)
  {
    // If we already have this IP discovered, ignore the packet
    if ((audioIPDiscovered && Udp.remoteIP() == discoveredAudioIP) ||
        (visualIPDiscovered && Udp.remoteIP() == discoveredVisualIP))
    {
      // Clear buffer but don't process
      char packetBuffer[255];
      Udp.read(packetBuffer, packetSize);
      return;
    }

    // Read the packet into a buffer
    char packetBuffer[255];
    Udp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = 0; // Null-terminate the string

    // Process the OSC message
    OSCMessage msg;
    for (int i = 0; i < packetSize; i++)
    {
      msg.fill(packetBuffer[i]);
    }

    if (msg.fullMatch("/audioComputer/discover"))
    {
      // Extract sender's IP address
      discoveredAudioIP = Udp.remoteIP();
      audioIPDiscovered = true;

      // Send acknowledgment
      sendAcknowledgment(discoveredAudioIP, outPortAudio, "audio", user.c_str());

      if (DEBUG)
      {
        Serial.print("Received discovery message from audio computer at IP: ");
        Serial.println(discoveredAudioIP);
      }
    }
    else if (msg.fullMatch("/visualComputer/discover"))
    {
      // Extract sender's IP address
      discoveredVisualIP = Udp.remoteIP();
      visualIPDiscovered = true;

      // Send acknowledgment
      sendAcknowledgment(discoveredVisualIP, outPortVisual, "visual", user.c_str());

      if (DEBUG)
      {
        Serial.print("Received discovery message from visual computer at IP: ");
        Serial.println(discoveredVisualIP);
      }
    }
  }
}

// OSC message addressing
// "/arduino/acknowledgment" -> Acknowledgment message
// "/debug" -> Debug message
// "/pulse" -> Pulse detection message
// "/hr/value" -> Heart rate message
// "/hr/min" -> Minimum heart rate message
// "/hr/max" -> Maximum heart rate message
// "/hr/interval" -> Heart rate interval message
// "/hrv/rmssd" -> Root mean square of successive differences
// "/hrv/rmssdMin" -> Minimum RMSSD value
// "/hrv/rmssdMax" -> Maximum RMSSD value
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

// HRV reset function
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

  // Reset NN intervals
  for (unsigned long i = 0; i < HRV_BUFFER_SIZE; i++)
  {
    NN_intervals[i] = 0;
    g_intervals_ms[i] = 0;
    g_cleaned_intervals[i] = 0;
    g_filtered_intervals[i] = 0;
  }
  NN_index = 0;
  hrvWindowStart = millis();
  initialHrvWindowFilled = false;
  HRV_calibrated = false; // Reset HRV calibration flag
  resetTimers();          // Reset timers for OSC communication
}

// Modified removeOutliers function to avoid VLAs
int removeOutliers(double *intervals, int size, double *cleaned)
{
  if (size < 4)
  {
    // Not enough data for quartile calculation, copy as is
    for (int i = 0; i < size; i++)
    {
      cleaned[i] = intervals[i];
    }
    return size;
  }

  // Use global array instead of local VLA
  // Create a copy for sorting
  for (int i = 0; i < size; i++)
  {
    g_filtered_intervals[i] = intervals[i]; // Reuse g_filtered_intervals as temp storage
  }

  // Perform a simple bubble sort (for small arrays, this is fine)
  for (int i = 0; i < size - 1; i++)
  {
    // Add watchdog reset during long operations
    if (i % 50 == 0)
    {
      esp_task_wdt_reset();
    }

    for (int j = 0; j < size - i - 1; j++)
    {
      if (g_filtered_intervals[j] > g_filtered_intervals[j + 1])
      {
        double temp = g_filtered_intervals[j];
        g_filtered_intervals[j] = g_filtered_intervals[j + 1];
        g_filtered_intervals[j + 1] = temp;
      }
    }
  }

  // Find Q1 and Q3
  int q1Idx = size / 4;
  int q3Idx = (3 * size) / 4;
  double q1 = g_filtered_intervals[q1Idx];
  double q3 = g_filtered_intervals[q3Idx];
  double iqr = q3 - q1;

  // Define bounds (1.5 is a common multiplier for outlier detection)
  double lowerBound = q1 - 1.5 * iqr;
  double upperBound = q3 + 1.5 * iqr;

  // Physiological limits for NN intervals (30-240 BPM converted to ms)
  double physiologicalLowerBound = 250;  // 240 BPM
  double physiologicalUpperBound = 2000; // 30 BPM

  // Use the more restrictive bounds
  lowerBound = max(lowerBound, physiologicalLowerBound);
  upperBound = min(upperBound, physiologicalUpperBound);

  // Filter outliers
  int newSize = 0;
  for (int i = 0; i < size; i++)
  {
    if (intervals[i] >= lowerBound && intervals[i] <= upperBound)
    {
      cleaned[newSize++] = intervals[i];
    }
  }

  return newSize;
}

// Modified gaussianFilterIntervals to use a fixed kernel size
void gaussianFilterIntervals(double *intervals, int size, double *filtered, int kernelSize, double sigma)
{
  // Create the Gaussian kernel with fixed size (avoid VLA)
  double kernel[7]; // Max kernel size of 7 should be enough
  double sum = 0.0;
  int actualKernelSize = min(kernelSize, 7);
  int halfSize = actualKernelSize / 2;

  // Calculate kernel values
  for (int i = 0; i < actualKernelSize; i++)
  {
    int x = i - halfSize;
    kernel[i] = exp(-(x * x) / (2 * sigma * sigma));
    sum += kernel[i];
  }

  // Safety check for sum
  if (sum <= 0.0)
  {
    sum = 1.0; // Prevent division by zero
  }

  // Normalize kernel
  for (int i = 0; i < actualKernelSize; i++)
  {
    kernel[i] /= sum;
  }

  // Apply filter with watchdog reset
  for (int i = 0; i < size; i++)
  {
    // Reset watchdog during lengthy calculations
    if (i % 50 == 0)
    {
      esp_task_wdt_reset();
    }

    filtered[i] = 0.0;
    double weightSum = 0.0;

    for (int j = -halfSize; j <= halfSize; j++)
    {
      int idx = i + j;
      if (idx >= 0 && idx < size)
      {
        filtered[i] += intervals[idx] * kernel[j + halfSize];
        weightSum += kernel[j + halfSize];
      }
    }

    // Normalize by actual weights used (for edge handling)
    if (weightSum > 0)
    {
      filtered[i] /= weightSum;
    }
    else
    {
      filtered[i] = intervals[i]; // Fallback if no weights were applied
    }
  }
}

// Modified computeHRVMetricsSliding to use global arrays
void computeHRVMetricsSliding()
{
  // First, disable interrupts while we work with the buffer
  noInterrupts();

  // Determine how many intervals to use (up to the buffer size)
  int n = min((int)NN_index, HRV_BUFFER_SIZE);

  // Create a local copy of intervals for calculations (using global array)
  for (int i = 0; i < n; i++)
  {
    g_intervals_ms[i] = NN_intervals[i];
  }

  // If we have more intervals than the buffer size, we need to manage them
  if (NN_index >= HRV_BUFFER_SIZE)
  {
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
    if (DEBUG)
    {
      Serial.println("Not enough data for HRV computation.");
    }
    else
    {
      // Send as OSC debug message
      sendOSCMessage("/debug/notEnoughData", 1.0);
    }
    return;
  }

  // Reset watchdog before heavy computations
  esp_task_wdt_reset();

  // Apply filtering to reduce noise and smooth NN intervals
  // 1. Remove outliers (using global arrays)
  int cleaned_size = removeOutliers(g_intervals_ms, n, g_cleaned_intervals);

  if (cleaned_size < 2)
  {
    if (DEBUG)
    {
      Serial.println("Not enough clean data for HRV computation after outlier removal.");
    }
    else
    {
      // Send as OSC debug message
      sendOSCMessage("/debug/notEnoughCleanData", 1.0);
    }
    return;
  }

  // Reset watchdog between heavy operations
  esp_task_wdt_reset();

  // 2. Apply Gaussian smoothing (using global arrays)
  gaussianFilterIntervals(g_cleaned_intervals, cleaned_size, g_filtered_intervals, kernelSize, sigma);

  // Use filtered data for calculations
  // Compute the mean of the filtered NN intervals
  double sum = 0;
  for (int i = 0; i < cleaned_size; i++)
  {
    sum += g_filtered_intervals[i];
  }
  double mean = sum / cleaned_size;

  // Reset watchdog periodically
  esp_task_wdt_reset();

  // Compute SDNN: standard deviation of all NN intervals.
  double sdnn_sum = 0;
  for (int i = 0; i < cleaned_size; i++)
  {
    double diff = g_filtered_intervals[i] - mean;
    sdnn_sum += diff * diff;
  }
  sdnn = sqrt(sdnn_sum / cleaned_size);

  // Reset watchdog
  esp_task_wdt_reset();

  // Compute RMSSD: square root of the mean squared differences of successive NN intervals.
  double rmssd_sum = 0;
  int count_diff = 0;
  for (int i = 0; i < cleaned_size - 1; i++)
  {
    double diff = g_cleaned_intervals[i + 1] - g_cleaned_intervals[i]; // Use cleaned intervals for RMSSD
    // double diff = g_filtered_intervals[i + 1] - g_filtered_intervals[i]; // Use filtered intervals for RMSSD
    rmssd_sum += diff * diff;
    count_diff++;
  }
  rmssd = (count_diff > 0) ? sqrt(rmssd_sum / count_diff) : 0;

  // Compute SDANN: approximate by dividing the window into 5 equal segments.
  int segments = 5;
  if (cleaned_size < segments * 2)
  {
    if (DEBUG)
    {
      Serial.println("Not enough data for SDANN computation.");
    }
    else
    {
      // Send as OSC debug message
      sendOSCMessage("/debug/notEnoughData", 1.0);
    }

    // Still compute SDANN with available data if possible
    segments = max(2, cleaned_size / 2);
  }

  int segSize = cleaned_size / segments;
  double segMeans[5];
  for (int s = 0; s < segments; s++)
  {
    double segSum = 0;
    int count = 0;
    for (int i = s * segSize; i < min((s + 1) * segSize, cleaned_size); i++)
    {
      segSum += g_filtered_intervals[i];
      count++;
    }
    segMeans[s] = (count > 0) ? segSum / count : 0;
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
    Serial.print("HRV Metrics (filtered): SDNN = ");
    Serial.print(sdnn);
    Serial.print(" ms, RMSSD = ");
    Serial.print(rmssd);
    Serial.print(" ms, SDANN = ");
    Serial.print(sdann);
    Serial.println(" ms");
    Serial.print("Original data points: ");
    Serial.print(n);
    Serial.print(", After filtering: ");
    Serial.println(cleaned_size);
    Serial.println();
  }
}
// --------------------------------------------- //

// ============================== //
// *********** Main ************* //
// ============================== //

void setup()
{
  // Initialize serial communication
  if (DEBUG)
  {
    // Initialize serial and wait for port to open
    Serial.begin(9600);
    delay(4000); // Give more time for serial to initialize
    Serial.println("Starting RESI PPG sensor...");
    Serial.println("Configuring device for stability...");
  }

  // Configure device for stability
  WiFiStabilityManager::configureDevice();

  if (DEBUG)
  {
    Serial.println("Debug mode enabled.");
    printMemInfo();

    Serial.println("Starting RESI WiFi detection and connection (IP discovery version)...");
  }

  // First detect network type
  currentNetworkType = detectNetworkType();

  // Connect to the appropriate network based on type
  bool wifiConnected = false;

  if (currentNetworkType == NETWORK_HOME)
  {
    if (DEBUG)
      Serial.println("Home network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }
  else if (currentNetworkType == NETWORK_STUDIO)
  {
    if (DEBUG)
      Serial.println("Studio network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
  }
  else if (currentNetworkType == NETWORK_CAMPUS)
  {
    if (DEBUG)
      Serial.println("Campus network detected");
    // Try campus connection (this will handle authentication too)
    if (handleUCBWirelessAuth())
    {
      if (DEBUG)
        Serial.println("Campus network authentication successful");
      wifiConnected = true;

      // Ensure we're stable before proceeding
      WiFiStabilityManager::waitForStableConnection();
    }
    else
    {
      if (DEBUG)
        Serial.println("Campus network authentication failed");
    }
  }
  else
  {
    if (DEBUG)
      Serial.println("Unknown network environment detected");
    // Try home network as default
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }

  if (DEBUG)
  {
    Serial.print("WiFi connection status: ");
    Serial.println(wifiConnected ? "Connected to: " : "Failed to connect");
    if (wifiConnected)
    {
      Serial.println(WiFi.SSID());
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
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

  esp_task_wdt_init(10, true); // 10 second timeout
}

void loop()
{
  // Listen for discovery messages from Max
  listenForDiscovery();

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
      if (DEBUG)
      {
        Serial.print("Network change detected: ");
        Serial.print(lastSSID);
        Serial.print(" -> ");
        Serial.println(currentSSID);
      }
      lastSSID = currentSSID;
    }

    // If WiFi is disconnected, try to reconnect without heavy operations
    if (WiFi.status() != WL_CONNECTED)
    {
      // Reset the watchdog timer to prevent resets during reconnection attempts
      esp_task_wdt_reset();

      if (DEBUG)
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

      // Check free heap space
      if (DEBUG)
        printMemInfo();

      // Send HRV metrics via OSC
      String addressRMSSD = user + "/hrv/rmssd";
      sendOSCMessage(addressRMSSD.c_str(), rmssd);

      String addressSDANN = user + "/hrv/sdann";
      sendOSCMessage(addressSDANN.c_str(), sdann);

      String addressSDNN = user + "/hrv/sdnn";
      sendOSCMessage(addressSDNN.c_str(), sdnn);
      // Update Min/Max values for RMSSD
      if (!HRV_calibrated)
      {
        rmssdMin = rmssd;
        rmssdMax = rmssd;
        String addressMin = user + "/hrv/rmssdMin";
        sendOSCMessage(addressMin.c_str(), rmssdMin);
        String addressMax = user + "/hrv/rmssdMax";
        sendOSCMessage(addressMax.c_str(), rmssdMax);

        HRV_calibrated = true;
      }
      else
      {
        if (rmssdMax < rmssd)
        {
          rmssdMax = rmssd;
          String addressMax = user + "/hrv/rmssdMax";
          sendOSCMessage(addressMax.c_str(), rmssdMax);
        }
        if (rmssdMin > rmssd)
        {
          rmssdMin = rmssd;
          String addressMin = user + "/hrv/rmssdMin";
          sendOSCMessage(addressMin.c_str(), rmssdMin);
        }
      }
    }
    if (newPulse)
    {
      newPulse = false;

      // Sending pulse message with static value indicating pulse detection
      String address = user + "/pulse";
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
          String address = user + "/hr/value";
          sendOSCMessage(address.c_str(), hRCurrentValue);

          String addressInterval = user + "/hr/interval";
          sendOSCMessage(addressInterval.c_str(), hRcurrent_millis);
        }

        if (!HR_calibrated)
        {
          hRUserMin = hRCurrentValue;
          hRUserMax = hRCurrentValue;
          HR_calibrated = true;
          String addressMin = user + "/hr/min";
          sendOSCMessage(addressMin.c_str(), hRUserMin);
          String addressMax = user + "/hr/max";
          sendOSCMessage(addressMax.c_str(), hRUserMax);
        }
        else
        {
          if (hRUserMax < hRCurrentValue)
          {
            hRUserMax = hRCurrentValue;
            String addressMax = user + "/hr/max";
            sendOSCMessage(addressMax.c_str(), hRUserMax);
          }
          if (hRUserMin > hRCurrentValue)
          {
            hRUserMin = hRCurrentValue;
            String addressMin = user + "/hr/min";
            sendOSCMessage(addressMin.c_str(), hRUserMin);
          }
        }
      }
      // Send pulse off message
      sendOSCMessage(address.c_str(), 0);
    }
  }
}
// --------------------------------------------- //

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// Connect to local UDP port for OSC communication
void connectLocalPort()
{
  delay(1000);
  if (DEBUG)
    Serial.println("Connecting to UDP port...");
  Udp.begin(inPort); // Local port to listen on (only matters for IP discovery, not much for sending)
  UDPConnected = true;
  if (DEBUG)
    Serial.println("UDP connection established!");
}

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Send to audio computer
  if (audioIPDiscovered)
  {
    Udp.beginPacket(discoveredAudioIP, outPortAudio);
  }
  else
  {
    Udp.beginPacket(outIpAudio, outPortAudio);
  }
  msg.send(Udp);
  Udp.endPacket();

  // Send to visual computer (use discovered IP if available)
  if (visualIPDiscovered)
  {
    Udp.beginPacket(discoveredVisualIP, outPortVisual);
  }
  else
  {
    Udp.beginPacket(outIpVisual, outPortVisual);
  }
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
    if (audioIPDiscovered)
    {
      Serial.print(discoveredAudioIP);
    }
    else
    {
      Serial.print(outIpAudio);
    }
    Serial.print("(audio) and ");
    if (visualIPDiscovered)
    {
      Serial.print(discoveredVisualIP);
    }
    else
    {
      Serial.print(outIpVisual);
    }
    Serial.println("(visual)");
  }
}
// --------------------------------------------- //
// End of file