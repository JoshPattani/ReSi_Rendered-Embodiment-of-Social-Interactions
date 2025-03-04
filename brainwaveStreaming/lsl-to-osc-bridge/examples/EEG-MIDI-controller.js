// EEG to MIDI Controller for Ableton Live
// Created for interfacing with OpenBCI Cyton board
// Handles brainwave data processing and mapping to MIDI parameters

inlets = 1;
outlets = 5; // MIDI control data, FFT data, Band Power, State Classification, Raw Signal

// Global Variables
var activeChannel = 0;
var numChannels = 8; // OpenBCI Cyton has 8 channels
var sampleRate = 250; // Hz
var bufferSize = 256;
var fftSize = 256;
var smoothingFactor = 0.8; // Default value (0-1)
var mappingMode = "linear"; // linear, exponential, logarithmic
var isRecording = false;
var trainingData = [];
var mlModel = null;
var currentState = "none";
var bandRanges = {
 delta: [0.5, 4],
 theta: [4, 8],
 alpha: [8, 13],
 beta: [13, 30],
 gamma: [30, 100],
};

// Initialize objects
var fft = new Max.JitterObject("jit.fft");
var buffer = new Max.JitterMatrix(1, "float32", bufferSize);
var eegBuffer = [];
for (var i = 0; i < numChannels; i++) {
 eegBuffer[i] = [];
}
var bandPower = {
 delta: Array(numChannels).fill(0),
 theta: Array(numChannels).fill(0),
 alpha: Array(numChannels).fill(0),
 beta: Array(numChannels).fill(0),
 gamma: Array(numChannels).fill(0),
};

// MIDI mapping settings
var midiMappings = {
 delta: { cc: 20, min: 0, max: 127, channel: 1 },
 theta: { cc: 21, min: 0, max: 127, channel: 1 },
 alpha: { cc: 22, min: 0, max: 127, channel: 1 },
 beta: { cc: 23, min: 0, max: 127, channel: 1 },
 gamma: { cc: 24, min: 0, max: 127, channel: 1 },
 rawActivity: { cc: 25, min: 0, max: 127, channel: 1 },
 mentalState: { cc: 26, min: 0, max: 127, channel: 1 },
};

// State classification settings
var states = ["relaxed", "focused", "excited", "meditative"];
var stateToMidiValue = {
 relaxed: 30,
 focused: 60,
 excited: 90,
 meditative: 120,
};

function msg_int(v) {
 // Handle integer input (likely channel selection)
 if (v >= 0 && v < numChannels) {
  activeChannel = v;
  post("Channel " + (activeChannel + 1) + " selected\n");
 }
}

function list() {
 // Process incoming EEG data
 var args = arrayfromargs(arguments);

 if (args.length > 0 && args[0] === "raw") {
  // Handle raw EEG data
  processRawData(args.slice(1));
 } else if (args.length > 0 && args[0] === "bandpower") {
  // Handle band power data directly from OpenBCI
  processBandPower(args.slice(1));
 } else if (args.length >= numChannels) {
  // Assume it's sample data for all channels
  for (var i = 0; i < numChannels && i < args.length; i++) {
   eegBuffer[i].push(args[i]);
   if (eegBuffer[i].length > bufferSize) {
    eegBuffer[i].shift();
   }
  }

  // Process the buffer when it's full
  if (eegBuffer[activeChannel].length >= bufferSize) {
   processSampleBuffer();
  }
 }
}

function processRawData(data) {
 // Handle raw EEG data
 if (data.length < numChannels) return;

 // Calculate signal activity (simple amplitude measure)
 var activity = 0;
 for (var i = 0; i < numChannels; i++) {
  activity += Math.abs(data[i]);
 }
 activity /= numChannels;

 // Map to MIDI CC
 var midiVal = mapValue(
  activity,
  0,
  100,
  midiMappings.rawActivity.min,
  midiMappings.rawActivity.max
 );

 // Send MIDI CC message for raw activity
 outlet(0, [
  "control",
  midiMappings.rawActivity.channel,
  midiMappings.rawActivity.cc,
  midiVal,
 ]);

 // Output raw signal to outlet 4
 outlet(4, data);
}

function processBandPower(data) {
 // Format expected: [channel, delta, theta, alpha, beta, gamma]
 if (data.length < 6) return;

 var channel = data[0];
 if (channel < 0 || channel >= numChannels) return;

 // Update band power values with smoothing
 bandPower.delta[channel] = smoothValue(bandPower.delta[channel], data[1]);
 bandPower.theta[channel] = smoothValue(bandPower.theta[channel], data[2]);
 bandPower.alpha[channel] = smoothValue(bandPower.alpha[channel], data[3]);
 bandPower.beta[channel] = smoothValue(bandPower.beta[channel], data[4]);
 bandPower.gamma[channel] = smoothValue(bandPower.gamma[channel], data[5]);

 // If this is the active channel, send MIDI CC messages
 if (channel === activeChannel) {
  sendBandPowerMidi(channel);

  // Also perform state classification if model is trained
  if (mlModel !== null) {
   classifyMentalState([
    bandPower.delta[channel],
    bandPower.theta[channel],
    bandPower.alpha[channel],
    bandPower.beta[channel],
    bandPower.gamma[channel],
   ]);
  }
 }

 // Output band power to outlet 2
 outlet(2, [
  "bandpower",
  channel,
  bandPower.delta[channel],
  bandPower.theta[channel],
  bandPower.alpha[channel],
  bandPower.beta[channel],
  bandPower.gamma[channel],
 ]);
}

function processSampleBuffer() {
 // Perform FFT on current buffer
 for (var i = 0; i < bufferSize; i++) {
  buffer.setcell(i, eegBuffer[activeChannel][i]);
 }

 fft.matrixcalc(buffer, buffer);

 // Extract band powers from FFT
 extractBandPowers();

 // Send FFT data to outlet
 outlet(1, "fft", buffer);

 // Send band powers to MIDI
 sendBandPowerMidi(activeChannel);
}

function extractBandPowers() {
 // Calculate band powers from FFT data
 var bands = ["delta", "theta", "alpha", "beta", "gamma"];

 for (var b = 0; b < bands.length; b++) {
  var band = bands[b];
  var freqStart = bandRanges[band][0];
  var freqEnd = bandRanges[band][1];

  // Convert frequency to FFT bin indices
  var binStart = Math.floor((freqStart * fftSize) / sampleRate);
  var binEnd = Math.ceil((freqEnd * fftSize) / sampleRate);

  // Sum power in the frequency range
  var power = 0;
  for (var i = binStart; i < binEnd && i < fftSize / 2; i++) {
   power += buffer.getcell(i);
  }

  // Normalize by number of bins and apply smoothing
  power /= binEnd - binStart;
  bandPower[band][activeChannel] = smoothValue(
   bandPower[band][activeChannel],
   power
  );
 }
}

function sendBandPowerMidi(channel) {
 // Send band powers as MIDI CC messages
 var bands = ["delta", "theta", "alpha", "beta", "gamma"];

 for (var b = 0; b < bands.length; b++) {
  var band = bands[b];
  var mapping = midiMappings[band];

  // Map band power to MIDI range using specified mapping mode
  var midiVal = mapValueWithMode(
   bandPower[band][channel],
   0,
   1, // Assuming normalized band power 0-1
   mapping.min,
   mapping.max,
   mappingMode
  );

  // Send MIDI CC message
  outlet(0, ["control", mapping.channel, mapping.cc, midiVal]);
 }
}

function classifyMentalState(features) {
 // Simple classifier based on band power features
 // In a real implementation, this would use a proper ML model

 if (!mlModel) {
  // Simple rule-based classification if no trained model
  if (features[2] > 0.6) {
   // High alpha
   currentState = "relaxed";
  } else if (features[3] > 0.7) {
   // High beta
   currentState = "focused";
  } else if (features[4] > 0.5) {
   // High gamma
   currentState = "excited";
  } else if (features[1] > 0.6) {
   // High theta
   currentState = "meditative";
  } else {
   currentState = "neutral";
  }
 } else {
  // Use trained ML model (placeholder for actual implementation)
  // currentState = mlModel.predict(features);
 }

 // Map state to MIDI value and send
 var midiVal = stateToMidiValue[currentState] || 0;
 outlet(0, [
  "control",
  midiMappings.mentalState.channel,
  midiMappings.mentalState.cc,
  midiVal,
 ]);

 // Output state to outlet 3
 outlet(3, ["state", currentState]);
}

function recordTrainingData(state) {
 // Start recording training data for the specified state
 if (!states.includes(state)) {
  post(
   "Invalid state: " + state + ". Valid states are: " + states.join(", ") + "\n"
  );
  return;
 }

 isRecording = true;
 currentTrainingState = state;
 post("Recording training data for state: " + state + "\n");
}

function stopRecording() {
 isRecording = false;
 post("Recording stopped\n");
}

function trainModel() {
 // Train the ML model with collected data
 // In a real implementation, this would use a proper ML library
 post("Training model with " + trainingData.length + " samples\n");

 // Placeholder for actual training
 mlModel = {};
 mlModel.trained = true;

 post("Model training complete\n");
}

function saveModel(filename) {
 // Save the trained model to a file
 if (!mlModel || !mlModel.trained) {
  post("No trained model to save\n");
  return;
 }

 // Placeholder for actual model saving
 post("Model saved to: " + filename + "\n");
}

function loadModel(filename) {
 // Load a trained model from a file
 // Placeholder for actual model loading
 mlModel = {};
 mlModel.trained = true;
 post("Model loaded from: " + filename + "\n");
}

function setMidiMapping(band, cc, min, max, channel) {
 // Update MIDI mapping settings
 if (midiMappings[band]) {
  midiMappings[band].cc = cc;
  midiMappings[band].min = min;
  midiMappings[band].max = max;
  midiMappings[band].channel = channel;
  post("Updated MIDI mapping for " + band + "\n");
 } else {
  post("Invalid band: " + band + "\n");
 }
}

function setSmoothingFactor(value) {
 // Set smoothing factor for band power
 if (value >= 0 && value <= 1) {
  smoothingFactor = value;
  post("Smoothing factor set to: " + value + "\n");
 } else {
  post("Smoothing factor must be between 0 and 1\n");
 }
}

function setMappingMode(mode) {
 // Set mapping mode for MIDI output
 var validModes = ["linear", "exponential", "logarithmic"];
 if (validModes.includes(mode)) {
  mappingMode = mode;
  post("Mapping mode set to: " + mode + "\n");
 } else {
  post("Invalid mapping mode. Valid modes: " + validModes.join(", ") + "\n");
 }
}

// Helper functions
function smoothValue(current, target) {
 return current * smoothingFactor + target * (1 - smoothingFactor);
}

function mapValue(value, inMin, inMax, outMin, outMax) {
 return ((value - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
}

function mapValueWithMode(value, inMin, inMax, outMin, outMax, mode) {
 var normalized = (value - inMin) / (inMax - inMin);

 switch (mode) {
  case "exponential":
   normalized = normalized * normalized;
   break;
  case "logarithmic":
   if (normalized > 0) {
    normalized = Math.log(normalized * 9 + 1) / Math.log(10);
   }
   break;
  case "linear":
  default:
   // No change for linear mapping
   break;
 }

 return normalized * (outMax - outMin) + outMin;
}
