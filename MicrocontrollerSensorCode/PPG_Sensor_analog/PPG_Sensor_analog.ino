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

// Configuration parameters
const int BAUDRATE = 19200;
const String USER = "A";
bool AnalogMode = true; // true for analog mode, false for digital interval mode
bool CHANNEL = false;   // true for channel B, false for channel A
bool CHANNELS = false;  // true for both channels, false for single channel

// Analog input pin assignments
const int analogPinA = A0; // Channel A
const int analogPinB = A1; // Channel B

// Digital input pin assignments (comparator outputs)
const int digitalPinA = 2; // Channel A (INT0)
const int digitalPinB = 3; // Channel B (INT1)

// Mode variable set by user over Serial
volatile char mode = ' ';

volatile bool analogSampleFlag = false; // Analog sampling parameters

// Digital mode timing variables (for pulse interval measurement)
volatile unsigned long lastTimeA = 0;
volatile unsigned long lastTimeB = 0;

// Timer1 Compare A ISR for analog sampling (500Hz)
ISR(TIMER1_COMPA_vect)
{
    analogSampleFlag = true;
}

// Function to set up Timer1 for analog sampling at 500Hz
void setupTimer1ForAnalogSampling()
{
    cli();      // Disable interrupts
    TCCR1A = 0; // Clear Timer/Counter Control Registers
    TCCR1B = 0;
    TCCR1B |= (1 << WGM12); // Configure Timer1 for CTC mode
    // Set prescaler to 8 for 16MHz clock: Timer frequency = 16MHz/8 = 2MHz
    TCCR1B |= (1 << CS11);
    OCR1A = 3999;            // Set compare value for 500Hz (4000 ticks - 1)
    TIMSK1 |= (1 << OCIE1A); // Enable Timer1 Compare A interrupt
    sei();                   // Enable interrupts
}

// Digital interval mode ISR for channel A
void sendIntervalA()
{
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastTimeA;
    lastTimeA = currentTime;
    Serial.print("A,");
    Serial.println(interval, HEX);
}

// Digital interval mode ISR for channel B
void sendIntervalB()
{
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastTimeB;
    lastTimeB = currentTime;
    Serial.print("B,");
    Serial.println(interval, HEX);
}

void setup()
{
    Serial.begin(BAUDRATE);
    Serial.println("PPG Sensor Multi-Participant System " + String(USER));

    if (AnalogMode)
    {
        Serial.println("Analog mode for:");
        if (CHANNELS)
        {
            Serial.println("Both channels A and B");
            mode = 'c';
        }
        else if (CHANNEL)
        {
            Serial.println("Channel B");
            mode = 'b';
        }
        else
        {
            Serial.println("Channel A");
            mode = 'a';
        }
        // For analog sampling mode, initialize Timer1 for 500Hz sampling.
        setupTimer1ForAnalogSampling();
    }
    else
    {
        Serial.println("Digital interval mode for:");
        if (CHANNELS)
        {
            Serial.println("Both channels A and B");
            mode = 'k';
            pinMode(digitalPinA, INPUT);
            attachInterrupt(digitalPinToInterrupt(digitalPinA), sendIntervalA, RISING);
            pinMode(digitalPinB, INPUT);
            attachInterrupt(digitalPinToInterrupt(digitalPinB), sendIntervalB, RISING);
        }
        else if (CHANNEL)
        {
            Serial.println("Channel B");
            mode = 'j';
            pinMode(digitalPinB, INPUT);
            attachInterrupt(digitalPinToInterrupt(digitalPinB), sendIntervalB, RISING);
        }
        else
        {
            Serial.println("Channel A");
            mode = 'i';
            pinMode(digitalPinA, INPUT);
            attachInterrupt(digitalPinToInterrupt(digitalPinA), sendIntervalA, RISING);
        }
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
    if (mode == 'a' || mode == 'b' || mode == 'c')
    {
        // In analog mode, check if a sample is due.
        if (analogSampleFlag)
        {
            analogSampleFlag = false;
            unsigned long timestamp = micros();
            if (mode == 'a')
            {
                int valueA = analogRead(analogPinA);
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",A,");
                Serial.println(valueA);
            }
            else if (mode == 'b')
            {
                int valueB = analogRead(analogPinB);
                Serial.print("T,");
                Serial.print(timestamp);
                Serial.print(",B,");
                Serial.println(valueB);
            }
            else if (mode == 'c')
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
    // In digital interval mode, interrupts handle the data output.
    // Additional processing can be added here if needed.
}
