#include <Arduino.h>
// If you use Arduino IoT Cloud, include your cloud header:
// #include "thingProperties.h"

const int sensorPin = 2; // Digital input pin for the heart rate sensor

// Variables for pulse timing and heart rate calculation
unsigned long pulseTimes[21]; // To hold timestamps for 21 pulses (0–20)
unsigned char pulseCount = 0; // Counter for pulses
unsigned int heartRate = 0;   // Calculated BPM (beats per minute)

const unsigned long maxInterval = 2000; // Maximum allowed interval between pulses (in ms)
bool dataEffect = true;                 // Flag indicating whether the data is valid

void setup(){
    Serial.begin(9600);

    // If using Arduino IoT Cloud, initialize your properties and cloud connection here:
    // initProperties();
    // ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    // while (!ArduinoCloud.connected()) {
    //   delay(100);
    // }

    pinMode(sensorPin, INPUT);

    // Initialize the pulseTimes array.
    for (int i = 0; i < 20; i++)
    {
        pulseTimes[i] = 0;
    }
    pulseTimes[20] = millis();
}

void loop(){
    // If using Arduino IoT Cloud, call update regularly:
    // ArduinoCloud.update();

    // Poll the sensor’s digital output.
    // We use a simple rising-edge detector.
    static bool previousState = LOW;
    bool currentState = digitalRead(sensorPin);

    if (!previousState && currentState)
    {                                      // Rising edge detected
        pulseTimes[pulseCount] = millis(); // Record current time

        // Determine the time interval from the previous pulse.
        unsigned long diff;
        if (pulseCount == 0)
        {
            // For the very first pulse, use the stored value in pulseTimes[20]
            diff = pulseTimes[pulseCount] - pulseTimes[20];
        }
        else
        {
            diff = pulseTimes[pulseCount] - pulseTimes[pulseCount - 1];
        }

        // If the interval is too long, reset the measurement.
        if (diff > maxInterval)
        {
            dataEffect = false;
            pulseCount = 0;
            Serial.println("Heart rate measure error, test will restart!");
            // Reinitialize the pulseTimes array.
            for (int i = 0; i < 20; i++)
            {
                pulseTimes[i] = 0;
            }
            pulseTimes[20] = millis();
        }
        else
        {
            // When 21 pulses have been collected, calculate BPM.
            if (pulseCount == 20 && dataEffect)
            {
                // Calculate BPM = 1200000 / (time difference between 1st and 21st pulse)
                heartRate = 1200000 / (pulseTimes[20] - pulseTimes[0]);
                Serial.print("Heart_rate_is: ");
                Serial.println(heartRate);
                // If using IoT Cloud, update the cloud property here, for example:
                // myHeartRateProperty = heartRate;
                pulseCount = 0;
            }
            else if (pulseCount != 20 && dataEffect)
            {
                pulseCount++;
            }
            else
            {
                // Reset state if data is not valid.
                pulseCount = 0;
                dataEffect = true;
            }
        }
    }

    previousState = currentState;

    // A short delay can help to reduce switch-bounce effects.
    delay(1);
}
