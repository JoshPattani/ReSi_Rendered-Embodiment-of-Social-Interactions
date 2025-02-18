const int sweatSensorPin = A0;
const int sampleSize = 5;  // Reduce the number of samples for less smoothing (try 2 or 3 as well)
int gSRCurrentValue = 0;
int gSRUserMax = 0;
int gSRUserMin = 1023;  // Initialize with the maximum possible value

int readings[sampleSize];  // Array to hold sensor readings for moving average
int readIndex = 0;  // Current index for the moving average
int total = 0;      // Running total for the moving average
int average = 0;    // Average value

void setup() {
  Serial.begin(9600);
  delay(1500);
  
  // Initialize readings array
  for (int i = 0; i < sampleSize; i++) {
    readings[i] = 0;
  }
}

void loop() {
  // Add new reading to the total
  total = total - readings[readIndex];  // Subtract the last reading
  readings[readIndex] = analogRead(sweatSensorPin);  // Get a new reading
  total = total + readings[readIndex];  // Add the new reading to total
  readIndex = (readIndex + 1) % sampleSize;  // Advance to the next index
  
  // Calculate the average of the readings
  average = total / sampleSize;
  
  // Update max and min values dynamically
  if (average < gSRUserMin) {
    gSRUserMin = average;
  }
  if (average > gSRUserMax) {
    gSRUserMax = average;
  }
  
  // You can try adding an offset or scaling the values differently here:
  // Example: map the value to a broader range
  gSRCurrentValue = map(average, 0, 1023, 0, 500);  // Broadening the range
  
  // Send values to serial monitor
  //Serial.print("Average: ");
  Serial.println(gSRCurrentValue);
  
  //Serial.print(" Min: ");
  //Serial.print(gSRUserMin);
  //Serial.print(" Max: ");
  //Serial.println(gSRUserMax);
  
  delay(500);  // Adjust delay to make fluctuations more visible
}

