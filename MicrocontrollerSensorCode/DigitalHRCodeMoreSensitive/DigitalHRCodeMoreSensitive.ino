const int sensorPin = 2; // Digital input pin for heart rate sensor
volatile unsigned long lastPulseTime = 0; // Stores the time of the last beat
volatile unsigned long pulseIntervals[10]; // Stores last 10 pulse intervals
volatile byte pulseIndex = 0; // Rolling index for interval storage
volatile bool newPulse = false; // Flag for new pulse detection
int hRCurrentValue = 0; // Current BPM value
int hRUserMax = -1000000;
int hRUserMin = 1000000;
bool calibrated = false;

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(sensorPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulseISR, RISING);

    // Initialize array with reasonable values (assume ~60 BPM at start)
    for (int i = 0; i < 10; i++) {
        pulseIntervals[i] = 1000;
    }
}

void loop() {
    if (newPulse) {
        newPulse = false;

        // Calculate rolling average BPM
        unsigned long sum = 0;
        for (int i = 0; i < 10; i++) {
            sum += pulseIntervals[i];
        }
        unsigned long avgInterval = sum / 10;

        if (avgInterval > 0) {
            hRCurrentValue = 60000 / avgInterval; // Convert ms to BPM
            Serial.print("Heart Rate: ");
            Serial.println(hRCurrentValue);

            if (!calibrated) {
                hRUserMin = hRCurrentValue;
                hRUserMax = hRCurrentValue;
                calibrated = true;
            } else {
                if (hRUserMax < hRCurrentValue) hRUserMax = hRCurrentValue;
                if (hRUserMin > hRCurrentValue) hRUserMin = hRCurrentValue;
            }
        }
    }
}

// Interrupt Service Routine (ISR) for detecting heartbeats
void pulseISR() {
    unsigned long currentTime = millis();
    unsigned long interval = currentTime - lastPulseTime;

    if (interval > 300 && interval < 2000) { // Ignore noise (too fast/slow beats)
        pulseIntervals[pulseIndex] = interval;
        pulseIndex = (pulseIndex + 1) % 10; // Move to next slot in rolling average array
        newPulse = true; // Notify main loop
    }

    lastPulseTime = currentTime;
}
