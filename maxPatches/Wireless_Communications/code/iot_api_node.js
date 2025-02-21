// Require built-in modules
const https = require("https");
const dgram = require("dgram");
const maxApi = require("max-api");
const config = require("./secret.json");
const { arduino } = config; // Extract Arduino credentials from secret.json

// Require third-party modules
const { ArduinoIoTCloud } = require("arduino-iot-js");
const ArduinoIotClient = require("@arduino/arduino-iot-client");
const fetch = require("node-fetch");

// ====================
// CONFIGURATION
// ====================

// Arduino IoT Cloud API endpoint.
// Replace YOUR_THING_ID with your actual Thing ID and adjust the URL as needed.
const API_URL = "https://api2.arduino.cc/iot/v1/clients/token";

// Polling interval in milliseconds
const POLL_INTERVAL = 1000; // 1 second

// OSC destination (e.g., where a udpreceive object in your patch is listening)
const OSC_IP = arduino.oscIP; // updated
const OSC_PORT = arduino.oscPort; // updated

// Create a UDP socket (IPv4)
const socket = dgram.createSocket("udp4");

// ====================
// HELPER FUNCTIONS
// ====================

/**
 * Gets an access token from the Arduino IoT Cloud API.
 * The token is stored in the global accessToken variable.
 */
async function getAccessToken() {
 try {
  const response = await fetch(API_URL, {
   method: "POST",
   url: API_URL,
   headers: {
    "Content-Type": "application/x-www-form-urlencoded",
   },
   form: {
    grant_type: "client_credentials",
    client_id: arduino.clientId, // updated
    client_secret: arduino.clientSecret, // updated
    audience: "https://api2.arduino.cc/iot",
   },
  });

  const data = await response.json();
  return data["access_token"];
 } catch (error) {
  maxApi.post(`Error obtaining access token: ${error.message}\n`);
 } finally {
  maxApi.post("Access token obtained successfully.\n");
 }
}

/**
 * Connects to the Arduino IoT Client API.
 * Listens to a thing's property changes.
 */
async function connectToArduinoIoTClient() {
 // Get the access token
 var client = ArduinoIotClient.ApiClient.instance;
 // Configure OAuth2 access token for authorization: oauth2
 var oauth2 = client.authentications["oauth2"];
 oauth2.accessToken = await getAccessToken(); // Changed to getAccessToken() function
 // Create an instance of the API class
 var apiInstance = new ArduinoIotClient.ThingsV2Api(client);
 var thingId = arduino.thingId; // String | The id of the thing
 var propertyId = arduino.variableId; // String | The id of the property
 var opts = {
  onMessage: function (data, response) {
   // Callback function that is called when a message is received
   console.log("onMessage called");
   console.log("Data: " + data);
   console.log("Response: " + response);
  },
 };
}

/**
 * Connects to the Arduino IoT Cloud API.
 * Listens to a thing's property changes.
 *
 */
async function connectToArduinoIoTCloud() {
 // Connect to the Arduino IoT Cloud API
 maxApi.post("Connecting to Arduino IoT Cloud API...\n");
 try {
  const client = await ArduinoIoTCloud.connect({
   clientId: arduino.clientId, // updated
   clientSecret: arduino.clientSecret, // updated
   // deviceId: arduino.deviceId,          // Uncomment if needed
   // secretKey: arduino.deviceSecret,     // Uncomment if needed
   onDisconnect: (message) => {
    maxApi.post("Disconnected from Arduino IoT Cloud: " + message + "\n");
    console.error(message);
   },
  });

  maxApi.post("Connected to Arduino IoT Cloud API.\n");
  client.onPropertyValue(arduino.thingId, arduino.variableName, (value) => {
   // updated
   maxApi.post(`Received value: ${value}\n`);
   processReceivedValue(value);
  });
 } catch (error) {
  maxApi.post(`Error connecting to Arduino IoT Cloud: ${error.message}\n`);
 }
}

/**
 * Declares the OSC address according to the sensor type.
 * Uses the arduino Thing ID and variable name after connecting to the Arduino IoT Cloud API.
 * Assigns the address to a variable for use in the OSC message.
 * @param {string} THING_ID - The name of the Thing.
 */
function declareOscAddress(THING_ID) {
 // Determine the sensor type based on the thing name
 let sensorType;
 // Split the Thing ID to extract the sensor type
 if (THING_ID.includes("GSR")) {
  sensorType = "GSR";
 } else if (THING_ID.includes("PPG")) {
  sensorType = "PPG";
 } else if (THING_ID.includes("Temperature")) {
  sensorType = "Temperature";
 } else {
  sensorType = "sensor";
 }

 // Assign the OSC address based on the sensor type
 const OSC_ADDRESS = "/" + sensorType + "_" + VARIABLE_NAME;

 return OSC_ADDRESS;
}

/**
 * Processes the received value.
 *
 * @param {number} value - The received value.
 */
function processReceivedValue(value) {
 // Create an OSC message with the appropriate address for a specific sensor.
 const address = declareOscAddress(arduino.thingId, arduino.variableName);
 const oscMsg = createOscMessage(address, value);

 maxApi.post("OSC Message created for " + address + "\n");

 // Send the OSC message via UDP.
 socket.send(oscMsg, 0, oscMsg.length, OSC_PORT, OSC_IP, (err) => {
  if (err) {
   maxApi.post("Error sending OSC message: " + err + "\n");
  } else {
   maxApi.post("Sent sensor value: " + value + "\n");
  }
 });
}

/**
 * Pads a string (with a trailing null) so its length is a multiple of 4.
 * OSC strings must be null-terminated and padded with zeros.
 *
 * @param {string} str - The string to pad.
 * @returns {Buffer} - The padded string as a Buffer.
 */
function padString(str) {
 // Add a null terminator
 let buf = Buffer.from(str + "\0");
 // Compute the number of zero bytes needed to reach a multiple of 4.
 const padBytes = (4 - (buf.length % 4)) % 4;
 if (padBytes > 0) {
  const pad = Buffer.alloc(padBytes, 0);
  buf = Buffer.concat([buf, pad]);
 }
 return buf;
}

/**
 * Creates a simple OSC message for an integer value.
 *
 * OSC message format:
 *   - OSC Address (e.g., "/sensor")
 *   - Type Tag String (e.g., ",i" for a 32-bit integer)
 *   - 32-bit big-endian integer value
 *
 * @param {string} address - The OSC address.
 * @param {number} value - The sensor value.
 * @returns {Buffer} - The complete OSC message.
 */
function createOscMessage(address, value) {
 // Create the padded OSC address.
 const addressBuf = padString(address);
 // Create the padded type tag. (Comma + "i" means one integer argument.)
 const typeTagBuf = padString(",i");
 // Allocate a 4-byte buffer for the integer value.
 const valueBuf = Buffer.alloc(4);
 valueBuf.writeInt32BE(value, 0);

 // Concatenate all parts into one Buffer.
 return Buffer.concat([addressBuf, typeTagBuf, valueBuf]);
}

/**
 * Polls the Arduino IoT Cloud API for the sensor value.
 * On success, builds an OSC message and sends it via UDP.
 */
async function pollApi() {
 // Connect to the Arduino IoT Cloud API
 await connectToArduinoIoTCloud();
 //  Sensor value updates are handled within connectToArduinoIoTCloud()
}

// ====================
// MAIN
// ====================
// Start polling the API
pollApi();

// Optionally, you can expose functions to MAX messages if desired.
// For example, to force a poll on a "bang" from MAX, you could define:
//
// exports.bang = function() {
//     pollApi();
// };
//
// In your MAX patch, sending a "bang" to the node.script object will call pollApi().
