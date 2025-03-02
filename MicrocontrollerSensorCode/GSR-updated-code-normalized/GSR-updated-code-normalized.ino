#include <WiFi.h>
#include <OSCMessage.h>

// Network configuration
WiFiUDP Udp;

// OSC configuration - this is where Max/MSP will listen
const IPAddress outIp(127, 0, 0, 1); // IP address of your computer running Max/MSP
const unsigned int outPort = 8000;   // OSC output port (match this in Max/MSP)
bool UDPConnected = false;           // Flag to check if UDP connection is established

const int sweatSensorPin = A0;
const int sampleSize = 5; // Reduce the number of samples for less smoothing (try 2 or 3 as well)
int gSRCurrentValue = 0;
int gSRUserMax = 0;
int gSRUserMin = 1023; // Initialize with the maximum possible value

int readings[sampleSize]; // Array to hold sensor readings for moving average
int readIndex = 0;        // Current index for the moving average
int total = 0;            // Running total for the moving average
int average = 0;          // Average value

// Function to print WiFi status for debugging
void printWiFiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// I discovered I need to wait for the WiFI connection to be established (WL_CONNECTED is true) before I create the UDP connection
void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  Udp.begin(42069); // Local port to listen on (doesn't matter much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// Callback function to run when connected to Arduino IoT Cloud
void doThisOnConnect()
{
  Serial.println("Connected to Arduino IoT Cloud!");
  printWiFiStatus();
  if (!UDPConnected)
  {
    connectLocalPort();
  }
}

void doThisOnDisconnect()
{
  Serial.println("Disconnected from Arduino IoT Cloud.");
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  delay(1000);
  UDPConnected = false;
}

void setup()
{
  Serial.begin(9600);
  delay(1000);
  Serial.println("Initializing GSR sensor with OSC output...");

  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

  // Initialize readings array
  for (int i = 0; i < sampleSize; i++)
  {
    readings[i] = 0;
  }
}

void loop()
{
  // Dont run if not connected to Arduino IoT Cloud
  if (!ArduinoCloud.connected() && !UDPConnected)
  {
    ArduinoCloud.update();
    return;
  }
  // Add new reading to the total
  total = total - readings[readIndex];              // Subtract the last reading
  readings[readIndex] = analogRead(sweatSensorPin); // Get a new reading
  total = total + readings[readIndex];              // Add the new reading to total
  readIndex = (readIndex + 1) % sampleSize;         // Advance to the next index

  // Calculate the average of the readings
  average = total / sampleSize;

  // Update max and min values dynamically
  if (average < gSRUserMin)
  {
    gSRUserMin = average;
  }
  if (average > gSRUserMax)
  {
    gSRUserMax = average;
  }

  // You can try adding an offset or scaling the values differently here:
  // Example: map the value to a broader range
  gSRCurrentValue = map(average, 0, 1023, 0, 500); // Broadening the range

  // Create OSC message
  OSCMessage msg("/gsr/value");
  msg.add(gSRCurrentValue);

  // Send the OSC message
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();

  // Clear the message
  msg.empty();

  // Send min/max values as additional OSC messages
  OSCMessage msgMin("/gsr/min");
  msgMin.add(gSRUserMin);
  Udp.beginPacket(outIp, outPort);
  msgMin.send(Udp);
  Udp.endPacket();
  msgMin.empty();

  OSCMessage msgMax("/gsr/max");
  msgMax.add(gSRUserMax);
  Udp.beginPacket(outIp, outPort);
  msgMax.send(Udp);
  Udp.endPacket();
  msgMax.empty();

  // Also print to Serial for debugging
  // Serial.print("GSR Value: ");
  // Serial.print(gSRCurrentValue);
  // Serial.print(" (Min: ");
  // Serial.print(gSRUserMin);
  // Serial.print(", Max: ");
  // Serial.print(gSRUserMax);
  // Serial.println(")");

  delay(500); // Adjust delay to make fluctuations more visible
}
