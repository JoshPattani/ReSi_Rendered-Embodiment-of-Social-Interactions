// Hello Potentiometer Hello Max
// https://www.tinkercad.com/things/l3OGBHcaqux-hello-potentiometer-max

// constants used to set pin numbers (don't change)
//const int ledPin = 13; //define ledPin and digtal pin 13 on arduino
//const int buttonPin = 2; //define button and digital pin 2 on arduino
const int sweatSensorPin = A0;
//const int hrSensorPin = A4; //define potentiometer and analog pin 0 on arduino

//int buttonState = 0; 
int sweatSensorVal = 0;
//int HR = 0;
int sweatMinVal = 10000000;
int sweatMaxVal = -1000000;

void setup() {
  Serial.begin(9600); //start serial data
  //pinMode(ledPin, OUTPUT); // initialize digital pin
  //pinMode(buttonPin, INPUT_PULLUP); // enable internal pull-up
}

void loop() {

//buttonState = digitalRead(buttonPin); // read state of button value
sweatSensorVal = analogRead(sweatSensorPin);
//HR = analogRead(hrSensorPin);
if (sweatSensorVal < sweatMinVal)
{
  sweatMinVal = sweatSensorVal;
}

if (sweatSensorVal > sweatMaxVal)
{
  sweatMaxVal = sweatSensorVal;
}
//sensorVal = map(sensorVal, 1941, 1955, 0, 255); //scale value
 
// check if pushbutton is pressed
  //
  Serial.print(" ");
  Serial.print(sweatSensorVal);
  Serial.print(" ");
  Serial.print(sweatMinVal);
  Serial.print(" ");
  Serial.print(sweatMaxVal);
  //Serial.print(" ");
  //Serial.print(HR);
  
  
  

  Serial.println(); //send serial to max 
}