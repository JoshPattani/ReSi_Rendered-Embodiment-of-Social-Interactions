/*
 * PPG_Sensor_analog.ino
 *
 * Arduino PPG Sensor Code for Multi-Participant Synchronization and Feature Extraction
 *
 * This sketch supports:
 *   - Analog "oscilloscope" mode: samples the full PPG waveform at 500Hz from one or two channels.
 *   - Digital interval mode: captures the time between rising edges of the PPG signal from one or two channels.
 *
 * Modes (enter one letter via Serial when the board starts):
 *   'a' : Analog mode, channel A only (PPG sensor A on analog pin A0)
 *   'b' : Analog mode, channel B only (PPG sensor B on analog pin A1)
 *   'c' : Analog mode, both channels (A and B)
 *   'i' : Digital interval mode, channel A only (PPG sensor A on digital pin 2)
 *   'j' : Digital interval mode, channel B only (PPG sensor B on digital pin 3)
 *   'k' : Digital interval mode, both channels (A and B)
 *
 * Data Formats:
 *   Analog mode: "T,<timestamp>,A,<value>" (or "T,<timestamp>,B,<value>" or "T,<timestamp>,A,<value>,B,<value>")
 *  Digital interval mode: "T,<timestamp>,A,<interval>" (or "T,<timestamp>,B,<interval>" or "T,<timestamp>,A,<interval>,B,<interval>")
 *
 * Note: For multi-MCU synchronization, ensure that all boards use a common time-reference or
 *       an external sync signal. Here we use micros() for local timestamps.
 *
 * Arduino IoT Cloud Variables description
 *
 *  The following variables are automatically generated and updated when changes are made to the Thing
 *
 *  int valueA;
 *  int valueB;
 *
 *  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
 *  which are called when their values are changed from the Dashboard.
 *  These functions are generated with the Thing and added at the end of this sketch.
 */

#include "thingProperties.h"
#include <math.h> // For sqrt()

// Configuration parameters
const int BAUDRATE = 19200;
const String USER = "A";

// Mode selection flags:
//   AnalogMode = true -> analog (oscilloscope) mode; false -> digital interval mode.
//   CHANNEL: false selects channel A; true selects channel B.
//   CHANNELS: true indicates both channels are used.
bool AnalogMode = false; // set to false for digital interval mode (required for HRV)
bool CHANNEL = false;    // false selects channel A (for HRV analysis)
bool CHANNELS = false;   // false means single channel

// Pin assignments
// For analog mode:
const int analogPinA = A0; // Channel A
const int analogPinB = A1; // Channel B

// For digital mode (e.g., comparator outputs):
const int digitalPinA = 2; // Channel A (INT0)
const int digitalPinB = 3; // Channel B (INT1)

// For analog sampling on ESP32 we use a hardware timer
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool analogSampleFlag = false; // Analog sampling parameters

// --- HRV data storage (digital mode) ---
// We assume a 5‑minute window. At high heart rates, you might see up to 600 beats.
// For simplicity, we use a fixed-size buffer.
#define HRV_BUFFER_SIZE 600
#define HRV_WINDOW_US 300000000UL // 5 minutes in microseconds

// Digital mode timing variables (for pulse interval measurement)
volatile unsigned long lastTimeA = 0;
volatile unsigned long lastTimeB = 0;

// HRV buffer and index
volatile unsigned long NN_intervals[HRV_BUFFER_SIZE];
volatile unsigned long NN_index = 0;
volatile unsigned long hrvWindowStart = 0;

// HRV metric variables
double sdnn, rmssd, sdann;

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

// --- Digital interval mode ISR for channel A ---
// Each rising edge calculates the interval (in µs) since the previous beat and stores it.
void IRAM_ATTR sendIntervalA()
{
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastTimeA;
    lastTimeA = currentTime;

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

    // Optionally, also output the interval over Serial:
    Serial.print("T,");
    Serial.print(currentTime);
    Serial.print(",A,");
    Serial.println(interval, HEX);
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

    Serial.print("T,");
    Serial.print(currentTime);
    Serial.print(",B,");
    Serial.println(interval, HEX);
}

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

    // Print the computed HRV metrics.
    Serial.print("HRV Metrics: SDNN = ");
    Serial.print(sdnn);
    Serial.print(" ms, RMSSD = ");
    Serial.print(rmssd);
    Serial.print(" ms, SDANN = ");
    Serial.print(sdann);
    Serial.println(" ms");
}

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
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",A,");
                Serial.println(valueA);
            }
            else if (CHANNEL)
            {
                int valueB = analogRead(analogPinB);
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",B,");
                Serial.println(valueB);
            }
            else if (CHANNELS)
            {
                int valueA = analogRead(analogPinA);
                int valueB = analogRead(analogPinB);
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
        }
    }
}
