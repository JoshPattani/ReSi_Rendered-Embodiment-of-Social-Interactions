/*
 * PPG_Sensor_analog.ino
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
 *    - SDANN: standard deviation of the average NN intervals computed over 5 segments within the 5‐minute window.
 *
 * The sketch accumulates NN intervals over a 5‑minute window (300,000,000 µs) and then computes
 * the HRV metrics. After computation, the buffer is reset to start a new window.
 *
 * WE ARE NO LONGER SENDING DATA TO THE CLOUD, BUT YOU CAN RE-ENABLE THIS FUNCTIONALITY.
 * THE SCRIPT IS BEING RECONFIGURED TO SEND DATA THROUGH OSC.
 *
 * The following variables are automatically generated and updated when changes are made to the Thing
 *
     float rmssd;
     float sdann;
     float sdnn;
     float valueA;
     float valueB;
     int hRCurrentValue;
     int hRUserMax;
     int hRUserMin;
 *
 * Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
 * which are called when their values are changed from the Dashboard.
 * These functions are generated with the Thing and added at the end of this sketch.
*/

#include "thingProperties.h"
#include <math.h> // For sqrt()
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "arduino_secrets.h"

// ================================ //
// ***** Configuration Params ***** //
// ================================ //

// Serial communication parameters
const int BAUDRATE = 19200;

// Debug flag
const bool DEBUG = true;

// !!! IMPORTANT !!!
// Move to environment variables or a separate header file before pushing to GitHub.
// !!!!!!!!!!!!!!!!!!
// Secret IP for OSC communication
#define SECRET_IP IPAddress(192, 168, 0, 8) // Target IP address (where your Max/MSP is running)
// Secret port for OSC communication
#define SECRET_PORT 12345 // Target port. Must match the port in your Max/MSP patch.
// !!!!!!!!!!!!!!!!!!

// Mode selection flags:
// User identifier (for multi-participant synchronization)
// Used for tagging data from different users in multi-user scenarios.
// Example:
// const String USER = "A"; // Participant A
// const String USER = "B"; // Participant B
const String USER = "A"; // Current participant identifier

//   AnalogMode = true -> analog (oscilloscope) mode; false -> digital interval mode.
//   CHANNEL: false selects channel A; true selects channel B.
//   CHANNELS: true indicates both channels are used.
bool AnalogMode = false; // set to false for digital interval mode (required for HRV)
bool CHANNEL = false;    // false selects channel A (for HRV analysis)
bool CHANNELS = false;   // false means single channel

// --------------------------------------------- //

// =============================== //
// ****** MCU Configuration ****** //
// =============================== //

// Analog pins for PPG sensor input:

// For analog mode (oscilloscope):
const int analogPinA = A0; // Channel A
const int analogPinB = A1; // Channel B

// For digital mode (e.g., comparator outputs):
const int digitalPinA = 2; // Channel A (INT0)
const int digitalPinB = 3; // Channel B (INT1)

// ================================= //
// ****** HR BPM  ****** //
// ================================= //

unsigned long lastBeatTimeA = 0;
unsigned long lastBeatTimeB = 0;
int heartRateA = 0;
int heartRateB = 0;

int calculateBPM()
{
    unsigned long sum = 0;
    for (int i = 0; i < 10; i++)
    {
        sum += NN_intervals[i];
    }
    unsigned long avgInterval = sum / 10;

    if (avgInterval > 0)
    {
        return 60000 / avgInterval; // Convert ms to BPM
    }
    return 0;
}

// --------------------------------------------- //

// =============================== //
// ****** HRV Data Storage ******* //
// =============================== //

// For analog sampling on ESP32 we use a hardware timer
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool analogSampleFlag = false; // Analog sampling parameters

// --- HRV data storage (digital mode) ---
// We assume a 5‑minute window. At high heart rates, you might see up to 600 beats.
// For simplicity, we use a fixed-size buffer. Modify as needed.
#define HRV_BUFFER_SIZE 600
#define HRV_WINDOW_US 300000000UL // 5 minutes in microseconds

// HRV metrics
float rmssd; // Root mean square of successive differences
float sdann; // Standard deviation of the average NN intervals
float sdnn;  // Standard deviation of NN intervals

// HRV buffer and index
// error for NN_intervals in function 'void resetHRVMetrics()': invalid conversion from 'volatile void*' to 'void*' [-fpermissive]
volatile unsigned long NN_intervals[HRV_BUFFER_SIZE];
volatile unsigned long NN_index = 0;
volatile unsigned long hrvWindowStart = 0;

// Flags for HRV computation and pulse detection
void resetHRVMetrics()
{
    hRCurrentValue = 0;
    hRUserMax = -1000000;
    hRUserMin = 1000000;

    newPulse = false;
    calibrated = false;

    // Initialize HRV metrics
    rmssd = 0;
    sdann = 0;
    sdnn = 0;

    for (unsigned long i = 0; i < HRV_BUFFER_SIZE; i++)
    {
        NN_intervals[i] = 0;
    }
    NN_index = 0;
    hrvWindowStart = micros();
    heartRateA = 0;
    heartRateB = 0;
    lastBeatTimeA = 0;
    lastBeatTimeB = 0;
}

// --------------------------------------------- //

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// OSC message addressing
// "/ppgA" for channel A in analog mode
// "/ppgB" for channel B in analog mode
// "/pulse" for pulse detection in digital mode
// "/heartRate" for heart rate in digital mode
// "/rmssd" for RMSSD in digital mode
// "/sdann" for SDANN in digital mode
// "/sdnn" for SDNN in digital mode
// "/sensor1" for sensor 1 in analog mode

// Target IP and port for OSC communication
// OSC configuration
const IPAddress targetIP = SECRET_IP;         // Target IP address (where your Max/MSP is running)
const unsigned int TARGET_PORT = SECRET_PORT; // Target port. Must match the port in your Max/MSP patch.

// Create a UDP instance for OSC communication
WiFiUDP Udp;

// Update intervals
const unsigned long oscSendInterval = 50;     // 50ms = 20Hz send rate
const unsigned long cloudSendInterval = 1000; // 1000ms = 1Hz cloud update rate

// Timers
unsigned long lastOscSendTime = 0;
unsigned long lastCloudSendTime = 0;

// Create and send OSC message
// OSC message handling
void sendOSCMessage(const char *address, float value)
{
    // Create an OSC message
    OSCMessage msg(address);
    msg.add(value);

    // Begin UDP packet
    Udp.beginPacket(targetIP, TARGET_PORT);

    // Write OSC message to UDP
    msg.send(udp);

    // End packet and send
    udp.endPacket();

    // Free space
    msg.empty();

    // Optionally print the sent message
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
// ****** Analog Sampling ******* //
// =============================== //

// --- Analog sampling on ESP32 ---
// ISR for the ESP32 hardware timer to trigger analog sampling at 500Hz
void IRAM_ATTR onTimer()
{
    portENTER_CRITICAL_ISR(&timerMux);
    analogSampleFlag = true;
    portEXIT_CRITICAL_ISR(&timerMux);
}

// Set up ESP32 hardware timer for analog sampling at 500Hz.
void setupTimerForAnalogSampling()
{
    timer = timerBegin(0, 80, true); // Prescaler 80 -> 1 µs per tick (for an 80MHz clock)
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 2000, true); // 2000 µs period => 500Hz
    timerAlarmEnable(timer);
}

// --------------------------------------------- //

// =============================== //
// ****** Digital Interval ******* //
// =============================== //

// --- Digital interval mode ISR for channel A ---
// Each rising edge calculates the interval (in µs) since the previous beat and stores it.
void IRAM_ATTR sendIntervalA()
{
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastTimeA;
    lastTimeA = currentTime;

    if (interval > 300 && interval < 2000)
    { // Ignore noise (too fast/slow beats)

        // Store the interval in the NN_intervals buffer.
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

        if (DEBUG)
        {
            // Optionally, also output the interval over Serial:
            Serial.print("T,");
            Serial.print(currentTime);
            Serial.print(",A,");
            Serial.println(interval, HEX);
        }

        // Set the newPulse flag to trigger heart rate calculation.
        newPulse = true;
    }
}

// --- Digital interval mode ISR for channel B ---
void IRAM_ATTR sendIntervalB()
{
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastTimeB;
    lastTimeB = currentTime;

    if (NN_index < HRV_BUFFER_SIZE)
    {
        NN_intervals[NN_index++] = interval;
    }
    else
    {
        NN_intervals[NN_index % HRV_BUFFER_SIZE] = interval;
        NN_index++;
    }

    if (DEBUG)
    {
        // Optionally, also output the interval over Serial:
        Serial.print("T,");
        Serial.print(currentTime);
        Serial.print(",B,");
        Serial.println(interval, HEX);
    }
}

// --------------------------------------------- //

// ============================== //
// ****** HRV Computation ******* //
// ============================== //

// --- HRV Computation ---
// This function computes SDNN, RMSSD, and SDANN from the NN_intervals collected in the current window.
// The intervals are first converted from microseconds to milliseconds.
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

void setup()
{
    Serial.begin(BAUDRATE);
    Serial.println("PPG Sensor Multi-Participant System " + String(USER));

    // For HRV feature extraction, we use digital interval mode on channel A.
    if (!AnalogMode && !CHANNEL)
    {
        Serial.println("Digital interval mode selected for channel A (HRV analysis).");
        // Set up digital pin and attach interrupt for channel A.
        pinMode(digitalPinA, INPUT);
        attachInterrupt(digitalPinA, sendIntervalA, RISING);
        // Initialize the last beat time.
        lastTimeA = micros();
        // Set the HRV window start time.
        hrvWindowStart = micros();

        // Initialize HRV metrics.
        resetHRVMetrics();
    }
    else if (AnalogMode)
    {
        // (Analog mode handling remains unchanged.)
        if (!CHANNELS)
        {
            if (CHANNEL)
            {
                Serial.println("Analog mode selected for channel B");
                // mode = 'b';
                pinMode(analogPinB, INPUT);
            }
            else
            {
                Serial.println("Analog mode selected for channel A");
                // mode = 'a';
                pinMode(analogPinA, INPUT);
            }
            setupTimerForAnalogSampling();
        }
        else
        {
            Serial.println("Analog mode selected for both channels A and B");
            // mode = 'c';
            pinMode(analogPinA, INPUT);
            pinMode(analogPinB, INPUT);
            setupTimerForAnalogSampling();
        }
    }
    else if (!AnalogMode && CHANNEL)
    {
        Serial.println("Digital interval mode selected for channel B");
        // mode = 'j';
        pinMode(digitalPinB, INPUT);
        attachInterrupt(digitalPinB, sendIntervalB, RISING);
    }
    else if (!AnalogMode && CHANNELS)
    {
        Serial.println("Digital interval mode selected for both channels A and B");
        // mode = 'k';
        pinMode(digitalPinA, INPUT);
        pinMode(digitalPinB, INPUT);
        attachInterrupt(digitalPinA, sendIntervalA, RISING);
        attachInterrupt(digitalPinB, sendIntervalB, RISING);
    }
    else
    {
        Serial.println("Invalid mode selection. Check configuration booleans.");
    }

    // Defined in thingProperties.h
    initProperties();

    // Begin UDP communication for OSC
    Udp.begin(TARGET_PORT); // Bind local UDP port (here we use the memeport)

    // Connect to Arduino IoT Cloud
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);

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
    ArduinoCloud.update();

    // In analog mode, the timer ISR and loop() handle sampling.
    if (AnalogMode)
    {
        if (analogSampleFlag)
        {
            portENTER_CRITICAL(&timerMux);
            analogSampleFlag = false;
            portEXIT_CRITICAL(&timerMux);

            unsigned long timestamp = micros();
            if (!CHANNEL)
            {
                int valueA = analogRead(analogPinA);
                sendOSCMessage("/ppgA", valueA);
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",A,");
                Serial.println(valueA);
            }
            else if (CHANNEL)
            {
                int valueB = analogRead(analogPinB);
                sendOSCMessage("/ppgB", valueB);
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",B,");
                Serial.println(valueB);
            }
            else if (CHANNELS)
            {
                int valueA = analogRead(analogPinA);
                int valueB = analogRead(analogPinB);
                sendOSCMessage("/ppgA", valueA);
                sendOSCMessage("/ppgB", valueB);
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",A,");
                Serial.print(valueA);
                Serial.print(",B,");
                Serial.println(valueB);
            }
        }
    }
    else
    {
        // In digital mode, the beat intervals are captured in the ISR.
        // Check if the current window (5 minutes) has elapsed.
        unsigned long now = micros();
        if ((now - hrvWindowStart) >= HRV_WINDOW_US)
        {
            // Compute and output HRV metrics for the current window.
            computeHRVMetrics();
            // (The computeHRVMetrics() function resets the window start and clears the buffer.)

            // Send the HRV metrics through OSC.
            sendOSCMessage("/rmssd", rmssd);
            sendOSCMessage("/sdann", sdann);
            sendOSCMessage("/sdnn", sdnn);
        }

        // In digital mode, the loop() function is used for cloud updates.
        if (newPulse)
        {
            // Sending pulse message with static value indicating pulse detection
            sendOSCMessage("/pulse", 1);

            // Update the cloud variable with the latest BPM value.
            hRCurrentValue = calculateBPM(); // Assuming calculateBPM() is a function that computes the latest BPM

            // Update the user's max and min heart rate values.
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

            if (DEBUG)
            {
                // Optionally, also output the BPM over Serial:
                Serial.print("Heart Rate: ");
                Serial.println(hRCurrentValue);
                Serial.print("User Max Heart Rate: ");
                Serial.println(hRUserMax);
                Serial.print("User Min Heart Rate: ");
                Serial.println(hRUserMin);
            }

            newPulse = false;
        }
    }

    // Send OSC messages at specified interval
    unsigned long currentTime = millis();
    if (currentTime - lastOscSendTime >= oscSendInterval)
    {
        lastOscSendTime = currentTime;
        // Send OSC message with the latest BPM value
        sendOSCMessage("/heartRate", hRCurrentValue);
        sendOSCMessage("/userMaxHR", hRUserMax);
        sendOSCMessage("/userMinHR", hRUserMin);
    }
}
