// OSC Receiver for Max/MSP
// This script receives OSC messages from ESP32 controllers and outputs them to Max

const maxApi = require("max-api");
const dgram = require("dgram");
const osc = require("node-osc");

// Create UDP server to receive OSC messages
const PORT = 12345;
const server = new osc.Server(PORT);

maxApi.post(`OSC server listening on port ${PORT}`);

// Handle incoming OSC messages
server.on("message", function (msg, rinfo) {
 const address = msg[0];
 const value = msg[1];

 maxApi.post(
  `Received OSC: ${address} ${value} from ${rinfo.address}:${rinfo.port}`
 );

 // Output the address and value as a list to Max
 maxApi.outlet([address, value]);
});

// Handle errors
server.on("error", (err) => {
 maxApi.post(`Server error: ${err.message}`);
});

// Handle Max/MSP script closing
maxApi.addHandler("close", () => {
 server.close();
 maxApi.post("OSC server closed");
});

// Add handler to change listening port if needed
maxApi.addHandler("port", (newPort) => {
 server.close();
 server = new osc.Server(newPort);
 maxApi.post(`OSC server now listening on port ${newPort}`);
});
