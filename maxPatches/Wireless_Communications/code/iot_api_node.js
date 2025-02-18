// Require built-in modules
const https = require("https");
const dgram = require("dgram");
const maxApi = require("max-api");

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

// Replace with your actual API token.

// Arduino IoT Client API credentials.
const CLIENT_ID = "XUg1anbdoKyu5tHu4AL0CqobAKsl9CN5";
const CLIENT_SECRET =
 "bOucUKZewfYnxkbPuItzVuZ0JXV61GJUpgQAAiHH7jCwPYjAN9rbxODhWhxHcl5m";

// Arduino IoT Cloud credentials.
const DEVICE_ID = "67cd42e1-e386-4a17-a8b7-e4daf07c5d72";
const DEVICE_SECRET = "V3t@qC6XzKPF1WHsAhSCJxtW@";

// Thing and variable IDs.
const THING_ID = "9f817909-9317-4b6f-82dd-02620591f54b";
const VARIABLE_ID = "9d55b47c-8b95-42b6-8634-2c24bb614fa0";
const VARIABLE_NAME = "pulseTrigger";

// Polling interval in milliseconds
const POLL_INTERVAL = 1000; // 1 second

// OSC destination (e.g., where a udpreceive object in your patch is listening)
const OSC_IP = "192.168.0.8";
const OSC_PORT = 12345;

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
    client_id: CLIENT_ID,
    client_secret: CLIENT_SECRET,
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
 var thingId = THING_ID; // String | The id of the thing
 var propertyId = VARIABLE_ID; // String | The id of the property
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
   clientId: CLIENT_ID,
   clientSecret: CLIENT_SECRET,
   //     deviceId: DEVICE_ID,
   //    secretKey: DEVICE_SECRET,
   onDisconnect: (message) => {
    maxApi.post("Disconnected from Arduino IoT Cloud: " + message + "\n");
    console.error(message);
   },
  });

  maxApi.post("Connected to Arduino IoT Cloud API.\n");
  client.onPropertyValue(THING_ID, VARIABLE_NAME, (value) => {
   maxApi.post(`Received value: ${value}\n`);
   processReceivedValue(value);
  });
 } catch (error) {
  maxApi.post(`Error connecting to Arduino IoT Cloud: ${error.message}\n`);
 }
}

/**
 * Processes the received value.
 *
 * @param {number} value - The received value.
 */
function processReceivedValue(value) {
 // Create an OSC message with the address "/sensor".
 const oscMsg = createOscMessage("/sensor", value);

 maxApi.post("OSC Message created \n");

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
 //  await connectToArduinoIoTClient();

 //  const options = {
 //   uri: `https://api.arduino.cc/iot/v1/things/${THING_ID}/properties/${VARIABLE_ID}/value`,
 //   method: "GET",
 //   json: true,
 //   headers: {
 //    Authorization: `Bearer ${await getAccessToken()}`,
 //   },
 //  };

 //  try {
 //   const response = await fetch(options.uri, {
 //    method: options.method,
 //    headers: options.headers,
 //   });

 //   const data = await response.json();
 //   const sensorValue = data.value;
 //   processReceivedValue(sensorValue);
 //  } catch (error) {
 //   maxApi.post(`Error polling API: ${error.message}\n`);
 //  }
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
