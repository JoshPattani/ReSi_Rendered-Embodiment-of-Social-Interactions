/*
 * PPG-updated-digiRead_OSC.ino
 *
 * Arduino PPG Sensor Code for Multi-Participant Synchronization, HRV Feature Extraction,
 * OSC Message Communication and IoT Cloud integration.
 *
 * This sketch supports:
 *   - Analog "oscilloscope" mode: samples the full PPG waveform at 500Hz.
 *   - Digital interval mode: captures the time between rising edges (NN intervals) of the PPG signal.
 *
 * Data Formats:
 *   Analog mode: "T,<timestamp>,A,<value>" (or similar for channel B or both channels)
 *   Digital interval mode: "T,<timestamp>,A,<interval>" (interval in hexadecimal)
 *
 * Note: For multi-MCU synchronization, ensure that all boards use a common time-reference or
 *       an external sync signal. Here we use micros() for local timestamps.
 *
 * HRV Feature Extraction (digital mode only):
 *    - SDNN: standard deviation of all NN intervals in the current window.
 *    - RMSSD: square root of the mean squared difference of successive NN intervals.
 *    - SDANN: standard deviation of the average NN intervals computed over 5 segments within the 2.5‐minute window.
 *
 * The sketch accumulates NN intervals over a 2.5‑minute window (150,000,000 µs) and then computes
 * the HRV metrics. After computation, the buffer is reset to start a new window.
 *
 * The following variables are automatically generated and updated when changes are made to the Thing
 *
    float rmssd;
    float sdann;
    float sdnn;
    float hRCurrentValue;
    float hRUserMax;
    float hRUserMin;
 *
 * Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
 * which are called when their values are changed from the Dashboard.
 * These functions are generated with the Thing and added at the end of this sketch.
*/
#include <Arduino.h>
#include <stdint.h>
#include "thingProperties.h"
#include <math.h> // For sqrt()
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// ================================ //
// ***** Configuration Params ***** //
// ================================ //

// Secrets for OSC communication
#define SECRET_IP IPAddress(192, 168, 0, 8) // Target IP address (where your Max/MSP is running)
#define SECRET_PORT 42069                   // Target port. Must match the port in your Max/MSP patch.

// Mode selection flags:
// User identifier (for multi-participant synchronization) Used for tagging data from different users in multi-user scenarios.
const String USER_A = "A";  // Participant A
const String USER_B = "B";  // Participant B
const String USER = USER_A; // Current participant identifier

// Serial communication parameters
const int BAUDRATE = 115200;

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
#define HRV_WINDOW_US 150000000UL // 2.5 minutes in microseconds

// HRV metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
// float rmssd; // Root mean square of successive differences
// float sdann; // Standard deviation of the average NN intervals
// float sdnn;  // Standard deviation of NN intervals

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
    hrvWindowStart = micros();
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

// UDP connection
WiFiUDP udp;
bool UDPConnected = false;

// Update intervals
const unsigned long oscSendInterval = 50;      // 50ms = 20Hz send rate
const unsigned long cloudSendInterval = 1000;  // 1000ms = 1Hz cloud update rate
const unsigned long minMaxSendInterval = 1000; // 1000ms = 1Hz send rate

// Timers
unsigned long lastOscSendTime = 0;
unsigned long lastCloudSendTime = 0;
unsigned long lastMinMaxSendTime = 0; // timer for min/max updates

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
    // Create an OSC message
    OSCMessage msg(address);
    msg.add(value);

    // Begin UDP packet
    udp.beginPacket(SECRET_IP, SECRET_PORT);

    // Write OSC message to UDP
    msg.send(udp);

    // End packet and send
    udp.endPacket();

    // Free space
    msg.empty();

    if (DEBUG)
    {
        Serial.print("Sent OSC message: ");
        Serial.print(address);
        Serial.print(" ");
        Serial.println(value);
    }
}

// --------------------------------------------- //

// =============================== //
// ****** UDP Connection ****** //
// =============================== //

// Udp Client handling
void connectLocalPort()
{
    delay(1000);
    Serial.println("Connecting to UDP port...");
    udp.begin(SECRET_PORT); // Local port to listen on (doesn't matter much for sending)
    UDPConnected = true;
    Serial.println("UDP connection established!");
}

// Callback function to run when connected to Arduino IoT Cloud
void doThisOnConnect()
{
    Serial.println("Connected to Arduino IoT Cloud!");
    if (!UDPConnected)
    {
        connectLocalPort();
        Serial.println("Ready to send OSC messages");
    }
}

void doThisOnDisconnect()
{
    Serial.println("Disconnected from Arduino IoT Cloud.");
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    delay(1000);
    UDPConnected = false;
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
        intervals_ms[i] = NN_intervals[i] * 0.001; // convert µs to ms
    }
    // Reset the global buffer for the next window.
    NN_index = 0;
    // Reset the window start time.
    hrvWindowStart = micros();
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
        Serial.print("HRV Metrics: SDNN = ");
        Serial.print(sdnn);
        Serial.print(" ms, RMSSD = ");
        Serial.print(rmssd);
        Serial.print(" ms, SDANN = ");
        Serial.print(sdann);
        Serial.println(" ms");
    }
}

// --------------------------------------------- //

// =============================== //
// ****** Setup and Loop ******* //
// =============================== //

void setup()
{
    Serial.begin(BAUDRATE);
    delay(1500);

    // Initialize PPG sensor
    // For HRV, we use the digital pin for interval measurement.
    if (!AnalogMode)
    {
        pinMode(sensorPin, INPUT);
        attachInterrupt(digitalPinToInterrupt(sensorPin), pulseISR, RISING);
        // Initialize the last beat time.
        lastBeatTime = micros();
        // Set the HRV window start time.
        hrvWindowStart = micros();

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

    // Defined in thingProperties.h
    initProperties();

    // Connect to Arduino IoT Cloud
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

    // Optionally wait until the device is connected.
    // while (ArduinoCloud.connected() != true) {
    //   Serial.println("Connecting to Arduino IoT Cloud...");
    //   delay(500);
    // }
    Serial.println("Connected to Arduino IoT Cloud");
    /*
       The following function allows you to obtain more information
       related to the state of network and IoT Cloud connection and errors
       the higher number the more granular information you’ll get.
       The default is 0 (only errors).
       Maximum is 4
   */
    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();
}

void loop()
{
    // Handle cloud connection
    if (!ArduinoCloud.connected())
    {
        ArduinoCloud.update();
        return;
    }
    ArduinoCloud.update();

    // Beat intervals are captured in the ISR.
    // Check if the current window (2.5 minutes) has elapsed.
    unsigned long now = micros();
    if (!AnalogMode && (now - hrvWindowStart) >= HRV_WINDOW_US)
    {
        // Compute and output HRV metrics for the current window.
        computeHRVMetrics();
        // (The computeHRVMetrics() function resets the window start and clears the buffer.)

        // Send the HRV metrics through OSC.
        sendOSCMessage("/rmssd", rmssd);
        sendOSCMessage("/sdann", sdann);
        sendOSCMessage("/sdnn", sdnn);
    }
    if (newPulse)
    {
        newPulse = false;

        // Sending pulse message with static value indicating pulse detection
        sendOSCMessage("/pulse", 1);

        // Update the cloud variable with the latest BPM value.
        hRCurrentValue = calculateBPM(); // Assuming calculateBPM() is a function that computes the latest BPM

        if (hRCurrentValue > 0)
        {
            Serial.print("Heart Rate: ");
            Serial.println(hRCurrentValue);

            if (!calibrated)
            {
                hRUserMin = hRCurrentValue;
                hRUserMax = hRCurrentValue;
                calibrated = true;
            }
            else
            {
                if (hRUserMax < hRCurrentValue)
                    hRUserMax = hRCurrentValue;
                if (hRUserMin > hRCurrentValue)
                    hRUserMin = hRCurrentValue;
            }
        }
    }

    // Dynamic min/max updates bases on change in values
    if (millis() - lastMinMaxSendTime >= minMaxSendInterval)
    {
        lastMinMaxSendTime = millis();
        if (hRCurrentValue > 0)
        {
            sendOSCMessage("/minHeartRate", hRUserMin);
            sendOSCMessage("/maxHeartRate", hRUserMax);
        }
    }

    unsigned long currentTime = millis();

    // Send OSC messages at specified interval
    if (currentTime - lastOscSendTime >= oscSendInterval)
    {
        lastOscSendTime = currentTime;
        sendOSCMessage("/heartRate", hRCurrentValue);
    }
}
