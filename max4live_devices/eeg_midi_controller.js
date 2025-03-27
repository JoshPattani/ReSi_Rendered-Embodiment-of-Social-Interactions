// EEG to MIDI Controller for Ableton Live
// Created for interfacing with OpenBCI Cyton board
// Receives raw EEG data and band power from the LSL-to-OSC bridger and outputs MIDI control messages
// Also includes basic state classification and training features
// Handles brainwave data processing and mapping to MIDI parameters

// Global Variables
inlets = 5; // Inlets for raw EEG data, band power data, and metrics data
outlets = 6; // MIDI control data, FFT data, Band Power, State Classification, Raw Signal
var activeChannel = 0;
var numChannels = 8; // OpenBCI Cyton has 8 channels
var sampleRate = 250; // Hz
var bufferSize = 256;
var fftSize = 256;
var smoothingFactor = 0.8; // Default value (0-1)
var highPassFreq = 0.5;
var lowPassFreq = 100;
var notchFilter = 0; // 0 = Off, 1 = 50Hz, 2 = 60Hz
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
var processAllChannels = true; // Default to processing all channels
var buffer = null;
var featureTimer = null;
var fft = null;
var currentTrainingState = "";
var currentTrainingClass = 0;
var oscDebug = false;
var lastProcessedTime = 0;

// Initialize for channel data
var eegBuffer = [];
for (var i = 0; i < numChannels; i++) {
 eegBuffer[i] = [];
}

// Band power data
var bandPower = {
 delta: new Array(numChannels),
 theta: new Array(numChannels),
 alpha: new Array(numChannels),
 beta: new Array(numChannels),
 gamma: new Array(numChannels),
};

// Initialize band power arrays
for (var i = 0; i < numChannels; i++) {
 bandPower.delta[i] = 0;
 bandPower.theta[i] = 0;
 bandPower.alpha[i] = 0;
 bandPower.beta[i] = 0;
 bandPower.gamma[i] = 0;
}

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

// ========================== Main Functions ==========================

// **************************
// ** Initialization **
// **************************

// Initialize buffer object
function initBuffer() {
 try {
  buffer = new JitterMatrix(1, "float32", bufferSize);
  post("Buffer initialized with size: " + bufferSize + "\n");
 } catch (e) {
  post("Error initializing buffer: " + e + "\n");
 }
}

// Initialize FFT object
function initFFT() {
 try {
  fft = new JitterObject("jit.fft");
  fft.dim = fftSize;
  post("FFT initialized with size: " + fftSize + "\n");
 } catch (e) {
  post("Error initializing FFT: " + e + "\n");
  // Fallback attempt with different parameters
  try {
   fft = new JitterObject("jit.fft");
   post("FFT initialized with default settings\n");
  } catch (e2) {
   post("Critical error: Could not initialize FFT: " + e2 + "\n");
  }
 }
}

// Initialize everything when the patch loads
function loadbang() {
 // Initialize data processing components
 initBuffer();
 initFFT();
 post("EEG MIDI Controller initialized\n");
}

// **************************
// ** Data Reception **
// **************************

// Function to enable/disable multi-channel processing
function setProcessAllChannels(enable) {
 processAllChannels = enable === 1 || enable === true;
 post(
  processAllChannels
   ? "Processing all channels enabled\n"
   : "Processing only active channel\n"
 );

 //
}

function msg_int(v) {
 // Handle integer input (likely channel selection)
 if (v >= 0 && v < numChannels) {
  activeChannel = v;
  post("Channel " + (activeChannel + 1) + " selected\n");
 } else {
  // All channels active
  activeChannel = 0;
  processAllChannels = true;
  post("All channels selected\n");
 }
}

// OSC Message Handlers
function anything() {
 // Log all incoming messages for debugging
 var args = arrayfromargs(arguments);
 if (args.length === 0) return;

 // Log for debugging
 var argString = "";
 for (var i = 0; i < args.length; i++) {
  argString += args[i] + " ";
 }
 post("anything() received: " + argString + "\n");

 // This function now just handles messages that aren't routed to specific handlers
 // You can add any custom message handling here if needed
}

// Function to handle raw EEG data
function processRawData(data) {
 // Check for valid data
 if (!data || data.length === 0) {
  post("Received empty raw data\n");
  return;
 }

 // Debug information
 post("Processing raw data with " + data.length + " values\n");

 // Parse and clip extreme values
 var numericData = [];
 for (var i = 0; i < data.length; i++) {
  var value = 0;
  if (typeof data[i] === "string") {
   value = parseFloat(data[i]);
  } else {
   value = data[i];
  }

  // Clip extreme values - signals often have huge outliers
  if (Math.abs(value) > 1000000) {
   value = Math.sign(value) * 1000000;
  }

  numericData.push(value);
 }

 // Store raw EEG data in buffer
 for (var i = 0; i < Math.min(numericData.length, numChannels); i++) {
  eegBuffer[i].push(numericData[i]);
  while (eegBuffer[i].length > bufferSize * 2) {
   // Keep twice the buffer size
   eegBuffer[i].shift();
  }
 }

 // Output raw signal to outlet 4 for visualization - use "rawdata" instead of "raw"
 outlet(4, ["rawdata"].concat(numericData));
 post("Sent raw EEG data to outlet 4\n");

 // Process FFT if we have enough data
 var shouldProcess = false;
 if (processAllChannels) {
  // Check if any channel has enough data
  for (var i = 0; i < numericData.length && i < numChannels; i++) {
   if (eegBuffer[i].length >= bufferSize) {
    shouldProcess = true;
    break;
   }
  }
 } else {
  // Check just the active channel
  shouldProcess = eegBuffer[activeChannel].length >= bufferSize;
 }

 if (shouldProcess) {
  post("Buffer has enough data, processing FFT\n");
  processSampleBuffer();
 } else {
  post(
   "Buffer not full yet (" +
    eegBuffer[activeChannel].length +
    "/" +
    bufferSize +
    ")\n"
  );
 }
}

// Update processBandsData to handle all channels
function processBandsData(address, value) {
 post("Processing band data: " + address + " = " + value + "\n");

 if (processAllChannels) {
  // In multi-channel mode, update all channels with the same value
  // (since the bridge sends the average across all channels)
  for (var i = 0; i < numChannels; i++) {
   if (address.indexOf("delta") !== -1) {
    bandPower.delta[i] = smoothValue(bandPower.delta[i], value);
   } else if (address.indexOf("theta") !== -1) {
    bandPower.theta[i] = smoothValue(bandPower.theta[i], value);
   } else if (address.indexOf("alpha") !== -1) {
    bandPower.alpha[i] = smoothValue(bandPower.alpha[i], value);
   } else if (address.indexOf("beta") !== -1) {
    bandPower.beta[i] = smoothValue(bandPower.beta[i], value);
   } else if (address.indexOf("gamma") !== -1) {
    bandPower.gamma[i] = smoothValue(bandPower.gamma[i], value);
   }
  }

  // Send MIDI for all channels
  for (var i = 0; i < numChannels; i++) {
   sendBandPowerMidi(i);
  }

  // Output band power data for the displayed channel
  outlet(2, [
   "bandpower",
   activeChannel,
   bandPower.delta[activeChannel],
   bandPower.theta[activeChannel],
   bandPower.alpha[activeChannel],
   bandPower.beta[activeChannel],
   bandPower.gamma[activeChannel],
  ]);
 } else {
  // Original single-channel mode
  if (address.indexOf("delta") !== -1) {
   bandPower.delta[activeChannel] = smoothValue(
    bandPower.delta[activeChannel],
    value
   );
  } else if (address.indexOf("theta") !== -1) {
   bandPower.theta[activeChannel] = smoothValue(
    bandPower.theta[activeChannel],
    value
   );
  } else if (address.indexOf("alpha") !== -1) {
   bandPower.alpha[activeChannel] = smoothValue(
    bandPower.alpha[activeChannel],
    value
   );
  } else if (address.indexOf("beta") !== -1) {
   bandPower.beta[activeChannel] = smoothValue(
    bandPower.beta[activeChannel],
    value
   );
  } else if (address.indexOf("gamma") !== -1) {
   bandPower.gamma[activeChannel] = smoothValue(
    bandPower.gamma[activeChannel],
    value
   );
  }

  // Update MIDI and output data
  sendBandPowerMidi(activeChannel);
  outlet(2, [
   "bandpower",
   activeChannel,
   bandPower.delta[activeChannel],
   bandPower.theta[activeChannel],
   bandPower.alpha[activeChannel],
   bandPower.beta[activeChannel],
   bandPower.gamma[activeChannel],
  ]);
 }
}

// Functions to directly process specific bands
function delta(v) {
 post("Direct delta: " + v + "\n");
 // Update all channels with the delta value
 for (var i = 0; i < numChannels; i++) {
  bandPower.delta[i] = smoothValue(bandPower.delta[i], v);
 }
 // Send updated band power data to outlet 2
 outlet(2, [
  "bandpower",
  activeChannel,
  bandPower.delta[activeChannel],
  bandPower.theta[activeChannel],
  bandPower.alpha[activeChannel],
  bandPower.beta[activeChannel],
  bandPower.gamma[activeChannel],
 ]);
 // Also update MIDI
 sendBandPowerMidi(activeChannel);
}

function theta(v) {
 post("Direct theta: " + v + "\n");
 // Update all channels with the theta value
 for (var i = 0; i < numChannels; i++) {
  bandPower.theta[i] = smoothValue(bandPower.theta[i], v);
 }
 // Same outputs as above
 outlet(2, [
  "bandpower",
  activeChannel,
  bandPower.delta[activeChannel],
  bandPower.theta[activeChannel],
  bandPower.alpha[activeChannel],
  bandPower.beta[activeChannel],
  bandPower.gamma[activeChannel],
 ]);
 sendBandPowerMidi(activeChannel);
}

function alpha(v) {
 post("Direct alpha: " + v + "\n");
 for (var i = 0; i < numChannels; i++) {
  bandPower.alpha[i] = smoothValue(bandPower.alpha[i], v);
 }
 outlet(2, [
  "bandpower",
  activeChannel,
  bandPower.delta[activeChannel],
  bandPower.theta[activeChannel],
  bandPower.alpha[activeChannel],
  bandPower.beta[activeChannel],
  bandPower.gamma[activeChannel],
 ]);
 sendBandPowerMidi(activeChannel);
}

function beta(v) {
 post("Direct beta: " + v + "\n");
 for (var i = 0; i < numChannels; i++) {
  bandPower.beta[i] = smoothValue(bandPower.beta[i], v);
 }
 outlet(2, [
  "bandpower",
  activeChannel,
  bandPower.delta[activeChannel],
  bandPower.theta[activeChannel],
  bandPower.alpha[activeChannel],
  bandPower.beta[activeChannel],
  bandPower.gamma[activeChannel],
 ]);
 sendBandPowerMidi(activeChannel);
}

function gamma(v) {
 post("Direct gamma: " + v + "\n");
 for (var i = 0; i < numChannels; i++) {
  bandPower.gamma[i] = smoothValue(bandPower.gamma[i], v);
 }
 outlet(2, [
  "bandpower",
  activeChannel,
  bandPower.delta[activeChannel],
  bandPower.theta[activeChannel],
  bandPower.alpha[activeChannel],
  bandPower.beta[activeChannel],
  bandPower.gamma[activeChannel],
 ]);
 sendBandPowerMidi(activeChannel);
}

// Add a function to handle metrics data from your bridge
function processMetricsData(address, value) {
 if (address.indexOf("mindfulness") !== -1) {
  // Map focus/mindfulness to a state
  if (value > 0.7) currentState = "focused";
  else if (value > 0.4) currentState = "neutral";

  // Output to state classification
  outlet(3, ["state", currentState]);

  // Map to MIDI CC
  var midiVal = mapValue(value, 0, 1, 0, 127);
  outlet(0, [
   "control",
   midiMappings.mentalState.channel,
   midiMappings.mentalState.cc,
   midiVal,
  ]);
 } else if (address.indexOf("restfulness") !== -1) {
  // Add mapping for relaxation/restfulness
  if (value > 0.7) currentState = "relaxed";
  else if (value > 0.5) currentState = "meditative";

  // Could map to a different CC if desired
 }
}

// Add explicit handlers for the OSC messages that are causing errors
function bandpower(args) {
 // Process bandpower data
 post("Received bandpower data\n");
 processBandPower(args);
}

function mindfulness(v) {
 post("Direct mindfulness: " + v + "\n");

 // Map focus/mindfulness to a state
 if (v > 0.7) currentState = "focused";
 else if (v > 0.4) currentState = "neutral";

 // Output to state classification
 outlet(3, ["state", currentState]);

 // Map to MIDI CC
 var midiVal = mapValue(v, 0, 1, 0, 127);
 outlet(0, [
  "control",
  midiMappings.mentalState.channel,
  midiMappings.mentalState.cc,
  midiVal,
 ]);
}

function restfulness(v) {
 post("Direct restfulness: " + v + "\n");

 // Map restfulness to states
 if (v > 0.7) currentState = "relaxed";
 else if (v > 0.5) currentState = "meditative";

 // Output state
 outlet(3, ["state", currentState]);
}

function focusMin(v) {
 // Store focus min value
 post("Focus Min: " + v + "\n");
}

function focusMax(v) {
 // Store focus max value
 post("Focus Max: " + v + "\n");
}

function relaxMin(v) {
 // Store relax min value
 post("Relax Min: " + v + "\n");
}

function relaxMax(v) {
 // Store relax max value
 post("Relax Max: " + v + "\n");
}

function timestamp(v) {
 // Process timestamp if needed
 post("Timestamp: " + v + "\n");
}

// Add a handler for EEG channels data
function eeg_channels() {
 var args = arrayfromargs(arguments);
 post("Direct EEG channels data with " + args.length + " values\n");

 // Process the raw data
 processRawData(args);

 // Update connection status
 outlet(3, ["status", "connected"]);
}

// function list() {
//  // Process incoming EEG data
//  var args = arrayfromargs(arguments);

//  if (args.length > 0 && args[0] === "raw") {
//   // Handle raw EEG data
//   processRawData(args.slice(1));
//  } else if (args.length > 0 && args[0] === "bandpower") {
//   // Handle band power data directly from OpenBCI
//   processBandPower(args.slice(1));
//  } else if (args.length >= numChannels) {
//   // Assume it's sample data for all channels
//   for (var i = 0; i < numChannels && i < args.length; i++) {
//    eegBuffer[i].push(args[i]);
//    if (eegBuffer[i].length > bufferSize) {
//     eegBuffer[i].shift();
//    }
//   }

//   // Process the buffer when it's full
//   if (eegBuffer[activeChannel].length >= bufferSize) {
//    processSampleBuffer();
//   }
//  }
// }

// function processBandPower(data) {
//  //   handles the data sent with [prepend bandpower]
//  // Format expected: [channel, delta, theta, alpha, beta, gamma]
//  if (data.length < 6) return;

//  var channel = data[0];
//  if (channel < 0 || channel >= numChannels) return;

//  // Update band power values with smoothing
//  bandPower.delta[channel] = smoothValue(bandPower.delta[channel], data[1]);
//  bandPower.theta[channel] = smoothValue(bandPower.theta[channel], data[2]);
//  bandPower.alpha[channel] = smoothValue(bandPower.alpha[channel], data[3]);
//  bandPower.beta[channel] = smoothValue(bandPower.beta[channel], data[4]);
//  bandPower.gamma[channel] = smoothValue(bandPower.gamma[channel], data[5]);

//  // If this is the active channel, send MIDI CC messages
//  if (channel === activeChannel) {
//   sendBandPowerMidi(channel);

//   // Also perform state classification if model is trained
//   if (mlModel !== null) {
//    classifyMentalState([
//     bandPower.delta[channel],
//     bandPower.theta[channel],
//     bandPower.alpha[channel],
//     bandPower.beta[channel],
//     bandPower.gamma[channel],
//    ]);
//   }
//  }

//  // Output band power to outlet 2
//  outlet(2, [
//   "bandpower",
//   channel,
//   bandPower.delta[channel],
//   bandPower.theta[channel],
//   bandPower.alpha[channel],
//   bandPower.beta[channel],
//   bandPower.gamma[channel],
//  ]);
// }

// **************************
// ** Data Processing & Analysis **
// **************************

// Replace the processSampleBuffer function with this multi-channel aware version:

function processSampleBuffer() {
 try {
  // Check if FFT is initialized
  if (!fft) {
   post("FFT is null, re-initializing...\n");
   initFFT();
   if (!fft) {
    post("Failed to initialize FFT, cannot process buffer\n");
    return;
   }
  }

  // Make sure buffer is initialized
  if (!buffer) {
   post("Buffer is null, re-initializing...\n");
   initBuffer();
   if (!buffer) {
    post("Failed to initialize buffer, cannot process sample\n");
    return;
   }
  }

  // Determine which channels to process
  var channelsToProcess = [];
  if (processAllChannels) {
   // Process all channels that have enough data
   for (var ch = 0; ch < numChannels; ch++) {
    if (eegBuffer[ch].length >= bufferSize) {
     channelsToProcess.push(ch);
    }
   }
  } else {
   // Process only active channel
   if (eegBuffer[activeChannel].length >= bufferSize) {
    channelsToProcess.push(activeChannel);
   }
  }

  post("Processing FFT for " + channelsToProcess.length + " channels\n");

  // If no channels have enough data, exit
  if (channelsToProcess.length === 0) {
   post("No channels have enough data for FFT processing\n");
   return;
  }

  // Process each channel
  for (var i = 0; i < channelsToProcess.length; i++) {
   var channel = channelsToProcess[i];

   // Fill buffer with EEG data for this channel
   for (var j = 0; j < bufferSize; j++) {
    if (j < eegBuffer[channel].length) {
     buffer.setcell(j, eegBuffer[channel][j] || 0);
    } else {
     buffer.setcell(j, 0);
    }
   }

   try {
    // Calculate FFT for this channel
    fft.matrixcalc(buffer, buffer);

    // Extract the FFT data as NUMERIC values
    var fftArray = [];
    for (var k = 0; k < Math.min(64, fftSize); k++) {
     try {
      // Extract as explicit float value
      var cellValue = parseFloat(buffer.getcell(k));

      // Check for NaN and replace with 0
      if (isNaN(cellValue)) {
       cellValue = 0;
      }

      fftArray.push(cellValue);
     } catch (e) {
      fftArray.push(0);
     }
    }

    // Send as array of numbers with channel identifier
    outlet(1, ["fft_data", channel].concat(fftArray));
    post("Sent FFT data for channel " + channel + "\n");

    // Extract band powers for this channel
    extractBandPowers(channel);

    // Output band powers to visualize
    var bandValues = [
     bandPower.delta[channel],
     bandPower.theta[channel],
     bandPower.alpha[channel],
     bandPower.beta[channel],
     bandPower.gamma[channel],
    ];

    // Send individual band powers to outlet for this channel
    outlet(5, ["fft_bandpower", channel].concat(bandValues));
    post("Sent FFT band powers for channel " + channel + "\n");

    // Send band powers to MIDI for this channel
    sendBandPowerMidi(channel);
   } catch (e) {
    post("Error in FFT calculation for channel " + channel + ": " + e + "\n");
    // Send dummy data if FFT fails
    var fftArray = [];
    for (var k = 0; k < 64; k++) {
     fftArray.push(Math.sin(k / 10) * 0.5 + 0.5);
    }
    outlet(1, ["fft_data", channel].concat(fftArray));
   }
  }
 } catch (e) {
  post("Error in processSampleBuffer: " + e + "\n");
 }
}

// Update extractBandPowers to accept a channel parameter

function extractBandPowers(channel) {
 // If no channel specified, use activeChannel
 channel = channel !== undefined ? channel : activeChannel;

 // Calculate band powers from FFT data
 var bands = ["delta", "theta", "alpha", "beta", "gamma"];
 var maxPower = 1.0; // Threshold for normalizing

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
   var binValue = buffer.getcell(i);
   // Sanity check for the bin value
   binValue = isNaN(binValue) || !isFinite(binValue) ? 0 : binValue;
   power += Math.min(1000, Math.abs(binValue)); // Limit extreme values
  }

  // Normalize by number of bins
  var binCount = Math.max(1, binEnd - binStart);
  power /= binCount;

  // Hard limit and normalize
  power = Math.min(maxPower, power) / maxPower;

  // Apply smoothing
  bandPower[band][channel] = smoothValue(bandPower[band][channel], power);
 }
}

function calculateOverallActivity(channel) {
 var channel = channel || activeChannel;
 var total = 0;
 var weights = {
  delta: 0.5, // Lower weight for slow waves
  theta: 0.8,
  alpha: 1.0,
  beta: 1.5, // Higher weight for fast waves
  gamma: 2.0, // Highest weight for gamma
 };

 // Calculate weighted sum of all band powers
 for (var band in bandPower) {
  total += bandPower[band][channel] * weights[band];
 }

 // Normalize to 0-1 range (assuming max possible value is around 5.0)
 var normalized = Math.min(total / 5.0, 1.0);

 // Send to MIDI
 var midiVal = mapValueWithMode(normalized, 0, 1, 0, 127, mappingMode);
 outlet(0, [
  "control",
  midiMappings.rawActivity.channel,
  midiMappings.rawActivity.cc,
  midiVal,
 ]);

 return normalized;
}

function extractFeatures() {
 // Extract features from current EEG data
 var features = [
  bandPower.delta[activeChannel],
  bandPower.theta[activeChannel],
  bandPower.alpha[activeChannel],
  bandPower.beta[activeChannel],
  bandPower.gamma[activeChannel],
  // Add more features like ratios
  bandPower.theta[activeChannel] / bandPower.beta[activeChannel],
  bandPower.alpha[activeChannel] / bandPower.beta[activeChannel],
 ];

 // Output features for Wekinator
 outlet(2, ["features"].concat(features));

 return features;
}

// **************************
// ** MIDI Output **
// **************************

// Update the sendMidiVolveControls function
function sendMidiVolveControls(channel) {
 channel = channel || activeChannel;

 // 1. Variation (Alpha/Theta)
 var variation = Math.max(bandPower.alpha[channel], bandPower.theta[channel]);
 variation = Math.max(0, Math.min(1, variation)); // Clamp to 0-1
 var variationCC = 30;
 var midiVal = mapValueWithMode(variation, 0, 1, 0, 127, mappingMode);
 outlet(0, ["control", 1, variationCC, midiVal]);

 // 2. Pattern Length (Beta)
 var patternLength = Math.max(0, Math.min(1, bandPower.beta[channel]));
 var patternLengthCC = 31;
 midiVal = mapValueWithMode(patternLength, 0, 1, 0, 127, mappingMode);
 outlet(0, ["control", 1, patternLengthCC, midiVal]);

 // 3. Rhythm Complexity (Delta/Gamma)
 var delta = Math.max(0, Math.min(1, bandPower.delta[channel]));
 var gamma = Math.max(0, Math.min(1, bandPower.gamma[channel]));
 var rhythmComplexity = delta * 0.3 + gamma * 0.7;
 rhythmComplexity = Math.max(0, Math.min(1, rhythmComplexity));
 var rhythmComplexityCC = 32;
 midiVal = mapValueWithMode(rhythmComplexity, 0, 1, 0, 127, mappingMode);
 outlet(0, ["control", 1, rhythmComplexityCC, midiVal]);

 // 4. Scale Selection (based on mental state) - no change needed as these are fixed values
 var scaleCC = 33;
 var scaleVal = 0;

 switch (currentState) {
  case "relaxed":
   scaleVal = 16; // Major scale
   break;
  case "focused":
   scaleVal = 32; // Minor scale
   break;
  case "excited":
   scaleVal = 48; // Pentatonic
   break;
  case "meditative":
   scaleVal = 64; // Lydian
   break;
  default:
   scaleVal = 0; // Chromatic
 }

 outlet(0, ["control", 1, scaleCC, scaleVal]);

 // 5. Randomization (overall activity)
 var overallActivity = calculateOverallActivity(channel);
 var randomizeCC = 34;
 midiVal = mapValueWithMode(overallActivity, 0, 1, 0, 127, mappingMode);
 outlet(0, ["control", 1, randomizeCC, midiVal]);
}

// Also update the sendBandPowerMidi function
function sendBandPowerMidi(channel) {
 var bands = ["delta", "theta", "alpha", "beta", "gamma"];

 for (var b = 0; b < bands.length; b++) {
  var band = bands[b];
  var mapping = midiMappings[band];

  // Ensure the band power is normalized before mapping
  var value = Math.max(0, Math.min(1, bandPower[band][channel]));

  var midiVal = mapValueWithMode(
   value,
   0,
   1,
   mapping.min,
   mapping.max,
   mappingMode
  );

  outlet(0, ["control", mapping.channel, mapping.cc, midiVal]);
 }
}

// Map value to MIDI range (0-127)
function mapValue(value, inMin, inMax, outMin, outMax) {
 return ((value - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
}

// Modify this function to ensure values stay in MIDI range
function mapValueWithMode(value, inMin, inMax, outMin, outMax, mode) {
 // First normalize and clamp to 0-1 range
 var normalized = (value - inMin) / (inMax - inMin);
 normalized = Math.max(0, Math.min(1, normalized));

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
   // No change for linear mapping
   break;
  default:
   break;
 }

 // Calculate output and ensure it's an integer in the valid range
 return Math.floor(
  Math.max(outMin, Math.min(outMax, normalized * (outMax - outMin) + outMin))
 );
}

// **************************
// Not used?
function updateMidiChannel(channel) {
 // Update MIDI channel for all mappings
 var bands = [
  "delta",
  "theta",
  "alpha",
  "beta",
  "gamma",
  "rawActivity",
  "mentalState",
 ];
 for (var i = 0; i < bands.length; i++) {
  midiMappings[bands[i]].channel = channel;
 }
 post("Updated all MIDI mappings to channel " + channel + "\n");
}

function setMidiMapping(band, cc, min, max, channel) {
 // Convert band name to lowercase to handle both "delta" and "Delta"
 var bandLower = band && typeof band === "string" ? band.toLowerCase() : band;

 // Update MIDI mapping settings
 if (midiMappings[bandLower]) {
  midiMappings[bandLower].cc = cc;
  midiMappings[bandLower].min = min;
  midiMappings[bandLower].max = max;
  midiMappings[bandLower].channel = channel;
  post("Updated MIDI mapping for " + bandLower + "\n");
 } else {
  post("Invalid band: " + band + "\n");
 }
}
// **************************

// **************************
// ** Machine Learning **
// **************************

function recordTrainingData(state) {
 // Replace includes with our helper function
 if (!arrayIncludes(states, state)) {
  post(
   "Invalid state: " + state + ". Valid states are: " + states.join(", ") + "\n"
  );
  return;
 }

 isRecording = true;
 currentTrainingState = state;
 startFeatureExtraction();

 // Send OSC message to Wekinator to start recording class
 var recordMessage = ["/wekinator/control/startRecording", state];
 outlet(4, recordMessage); // Assuming outlet 4 goes to an [udpsend] to Wekinator control port
 post("Recording training data for state: " + state + "\n");
}

// Enhance the trainModel function
function trainModel() {
 post("Training model with " + trainingData.length + " samples\n");

 if (trainingData.length < 10) {
  post("Not enough training data. Please record more samples.\n");
  return;
 }

 // Simple k-means clustering approach (can be implemented in JS)
 // Group the training data by state
 var stateData = {};
 for (var s = 0; s < states.length; s++) {
  stateData[states[s]] = [];
 }

 // Populate state data
 for (var i = 0; i < trainingData.length; i++) {
  var sample = trainingData[i];
  stateData[sample.state].push(sample.features);
 }

 // Calculate centroids for each state
 mlModel = { centroids: {}, trained: true };
 for (var state in stateData) {
  if (stateData[state].length === 0) continue;

  var centroid = [0, 0, 0, 0, 0]; // Delta, Theta, Alpha, Beta, Gamma
  for (var i = 0; i < stateData[state].length; i++) {
   for (var j = 0; j < 5; j++) {
    centroid[j] += stateData[state][i][j];
   }
  }

  // Average
  for (var j = 0; j < 5; j++) {
   centroid[j] /= stateData[state].length;
  }

  mlModel.centroids[state] = centroid;
 }

 post(
  "Model training complete - trained for states: " +
   Object.keys(mlModel.centroids).join(", ") +
   "\n"
 );
}

function classifyMentalState(features) {
 if (!mlModel || !mlModel.trained) {
  // Use the simple rule-based classification as fallback
  // This is already implemented in your code
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
  // Use the trained model
  var minDistance = Infinity;
  currentState = "neutral";

  for (var state in mlModel.centroids) {
   var centroid = mlModel.centroids[state];
   var distance = 0;

   // Calculate Euclidean distance
   for (var i = 0; i < 5; i++) {
    distance += Math.pow(features[i] - centroid[i], 2);
   }
   distance = Math.sqrt(distance);

   if (distance < minDistance) {
    minDistance = distance;
    currentState = state;
   }
  }
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

// function record(classNum) {
//  // Start sending training data for the specified class
//  // This just starts regular feature extraction, but marks the class
//  currentTrainingClass = classNum;
//  isRecording = true;
//  startFeatureExtraction();

//  // Send OSC message to Wekinator to start recording class
//  var recordMessage = ["/wekinator/control/startRecording", classNum];
//  outlet(4, recordMessage); // Assuming outlet 4 goes to an [udpsend] to Wekinator control port

//  post("Recording training data for class " + classNum + "\n");
// }

// function recordTrainingData(state) {
//  // Start recording training data for the specified state
//  if (!states.includes(state)) {
//   post(
//    "Invalid state: " + state + ". Valid states are: " + states.join(", ") + "\n"
//   );
//   return;
//  }

//  isRecording = true;
//  currentTrainingState = state;
//  post("Recording training data for state: " + state + "\n");
// }

// function stopRecording() {
//  isRecording = false;
//  post("Recording stopped\n");
// }

// Enhance classifyMentalState to use the trained model
// function classifyMentalState(features) {
//  if (!mlModel || !mlModel.trained) {
//   // Use the simple rule-based classification as fallback
//   // This is already implemented in your code
//  } else {
//   // Use the trained model
//   var minDistance = Infinity;
//   currentState = "neutral";

//   for (var state in mlModel.centroids) {
//    var centroid = mlModel.centroids[state];
//    var distance = 0;

//    // Calculate Euclidean distance
//    for (var i = 0; i < 5; i++) {
//     distance += Math.pow(features[i] - centroid[i], 2);
//    }
//    distance = Math.sqrt(distance);

//    if (distance < minDistance) {
//     minDistance = distance;
//     currentState = state;
//    }
//   }
//  }

//  // The rest of your existing code for mapping and output
// }

// Helper functions

function startFeatureExtraction() {
 if (featureTimer === null) {
  featureTimer = setInterval(extractFeatures, 50);
 }
}

function stopFeatureExtraction() {
 if (featureTimer !== null) {
  clearInterval(featureTimer);
  featureTimer = null;
 }
}

function stopRecording() {
 isRecording = false;
 currentTrainingClass = 0;

 // Send OSC message to Wekinator to stop recording
 var stopMessage = ["/wekinator/control/stopRecording"];
 outlet(4, stopMessage);

 post("Recording stopped\n");
}

// **************************
// ** Placeholder for Model Saving/Loading **
// ** To be implemented based on actual ML library **
// **************************

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

// **************************
// ** Utility & Debugging Functions **
// **************************

// ** Parameter Setting Functions **

function setSmoothingFactor(value) {
 // Set smoothing factor for band power
 if (value >= 0 && value <= 1) {
  smoothingFactor = value;
  post("Smoothing factor set to: " + value + "\n");
 } else {
  post("Smoothing factor must be between 0 and 1\n");
 }
}

function setFFTSize(size) {
 fftSize = parseInt(size);

 try {
  // Recreate FFT object with new size
  fft = new JitterObject("jit.fft");
  fft.dim = fftSize;
  post("FFT size set to " + fftSize + "\n");
 } catch (e) {
  post("Error setting FFT size: " + e + "\n");
 }
}

function setHighPass(freq) {
 // Update high-pass filter frequency
 highPassFreq = freq;
 post("High-pass filter set to " + freq + " Hz\n");
 // Recalculate band ranges based on filter settings
 updateBandRanges();
}

function setLowPass(freq) {
 // Update low-pass filter frequency
 lowPassFreq = freq;
 post("Low-pass filter set to " + freq + " Hz\n");
 // Recalculate band ranges based on filter settings
 updateBandRanges();
}

function setNotchFilter(setting) {
 // 0 = Off, 1 = 50Hz, 2 = 60Hz
 notchFilter = setting;
 post(
  "Notch filter set to " +
   (setting == 0 ? "Off" : setting == 1 ? "50Hz" : "60Hz") +
   "\n"
 );
}

function setMentalState(state) {
 // Map from Wekinator class number to state string
 var stateNames = ["none", "relaxed", "focused", "excited", "meditative"];
 currentState = stateNames[state];

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
function smoothValue(current, target) {
 return current * smoothingFactor + target * (1 - smoothingFactor);
}

function setMappingMode(mode) {
 // Set mapping mode for MIDI output
 var validModes = ["linear", "exponential", "logarithmic"];
 var isValid = false;

 for (var i = 0; i < validModes.length; i++) {
  if (validModes[i] === mode) {
   isValid = true;
   break;
  }
 }

 if (isValid) {
  mappingMode = mode;
  post("Mapping mode set to: " + mode + "\n");
 } else {
  post("Invalid mapping mode. Valid modes: " + validModes.join(", ") + "\n");
 }
}

function setupMidiVolveMapping() {
 // Store the original mappings without using Object.assign
 var originalMappings = {};
 for (var band in midiMappings) {
  originalMappings[band] = {
   cc: midiMappings[band].cc,
   min: midiMappings[band].min,
   max: midiMappings[band].max,
   channel: midiMappings[band].channel,
  };
 }

 // Update for MidiVolve-specific mappings
 midiMappings.delta.cc = 30; // For Rhythm Complexity
 midiMappings.theta.cc = 31; // For Variation
 midiMappings.alpha.cc = 32; // For Variation
 midiMappings.beta.cc = 33; // For Pattern Length
 midiMappings.gamma.cc = 34; // For Rhythm Complexity
 midiMappings.rawActivity.cc = 35; // For Randomization
 midiMappings.mentalState.cc = 36; // For Scale Selection

 post("MidiVolve mappings configured:\n");
 post("  Delta (CC 30): Rhythm Complexity\n");
 post("  Theta (CC 31): Variation\n");
 post("  Alpha (CC 32): Variation\n");
 post("  Beta (CC 33): Pattern Length\n");
 post("  Gamma (CC 34): Rhythm Complexity\n");
 post("  Overall Activity (CC 35): Randomization\n");
 post("  Mental State (CC 36): Scale Selection\n");
}

// Different mapping modes for different musical parameters
function setParameterResponseCurve(parameter, curveType) {
 var validParams = [
  "variation",
  "patternLength",
  "rhythmComplexity",
  "scale",
  "randomization",
 ];
 var validCurves = [
  "linear",
  "exponential",
  "logarithmic",
  "step",
  "threshold",
 ];

 if (
  validParams.indexOf(parameter) >= 0 &&
  validCurves.indexOf(curveType) >= 0
 ) {
  parameterCurves[parameter] = curveType;
  post("Set " + parameter + " response curve to " + curveType + "\n");
 } else {
  post("Invalid parameter or curve type\n");
 }
}

function enableMidiVolveMode(enable) {
 midiVolveMode = enable === 1 || enable === true;

 if (midiVolveMode) {
  setupMidiVolveMapping();
  post("MidiVolve direct mapping mode enabled\n");
 } else {
  // Restore standard mappings
  post("Standard MIDI mapping mode enabled\n");
 }
}

// ** Helper Functions **

function arrayIncludes(arr, value) {
 // Helper function to simulate Array.includes()
 for (var i = 0; i < arr.length; i++) {
  if (arr[i] === value) {
   return true;
  }
 }
 return false;
}

function updateBandRanges() {
 // Update frequency ranges based on filter settings
 bandRanges.delta[0] = Math.max(0.5, highPassFreq);
 bandRanges.delta[1] = Math.min(4, lowPassFreq);
 bandRanges.theta[0] = Math.max(4, highPassFreq);
 bandRanges.theta[1] = Math.min(8, lowPassFreq);
 bandRanges.alpha[0] = Math.max(8, highPassFreq);
 bandRanges.alpha[1] = Math.min(13, lowPassFreq);
 bandRanges.beta[0] = Math.max(13, highPassFreq);
 bandRanges.beta[1] = Math.min(30, lowPassFreq);
 bandRanges.gamma[0] = Math.max(30, highPassFreq);
 bandRanges.gamma[1] = Math.min(100, lowPassFreq);
}

// This function is already defined earlier in the code
// Commenting out the duplicate definition
/* 
// Add dedicated handler for raw EEG array data
function eeg_channels() {
 var args = arrayfromargs(arguments);
 post("Received raw EEG data with " + args.length + " channels\n");

 // Process the raw EEG data
 processRawData(args);

 // Also update the UI to show we're receiving data
 outlet(3, ["status", "connected"]);
}
*/

// Add dedicated handler for band powers
function avg_band_powers(value) {
 post("Received avg_band_powers: " + value + "\n");
 // This is already being handled by individual band handlers
}

// ** Testing functions **

// Debugging feature to see what's actually being received from OSC:
function showOscPaths(enable) {
 oscDebug = enable === 1 || enable === true;
 post("OSC path debugging " + (oscDebug ? "enabled" : "disabled") + "\n");
}

// Special debug function that shows all raw OSC messages
function showRawOSC() {
 // Set flag to show all incoming messages
 post("Raw OSC monitoring ENABLED\n");
 rawOSCmonitor = true;
}

// Fix forceUpdate to handle null buffer
function forceUpdate() {
 post("Forcing FFT update\n");

 // Make sure buffer is initialized
 if (!buffer) {
  initBuffer();
 }

 // Check if FFT is initialized
 if (!fft) {
  initFFT();
 }

 // Fill buffer with test sine wave data if needed
 if (eegBuffer[activeChannel].length < bufferSize) {
  // Clear existing data first to avoid mixing real and synthetic data
  eegBuffer[activeChannel] = [];

  for (var i = 0; i < bufferSize; i++) {
   // Generate a realistic EEG-like signal with multiple frequency components
   var value =
    100 * Math.sin(i / 10) + // Delta wave component
    50 * Math.sin(i / 3) + // Theta wave component
    25 * Math.sin(i); // Alpha wave component

   eegBuffer[activeChannel].push(value);
  }
  post("Filled buffer with test data\n");
 }

 processSampleBuffer();
}

// Debugging function to print current state
function debug() {
 post("-------- DEBUG INFO --------\n");
 post("Active Channel: " + activeChannel + "\n");
 post("Band Powers for active channel:\n");
 post("  Delta: " + bandPower.delta[activeChannel] + "\n");
 post("  Theta: " + bandPower.theta[activeChannel] + "\n");
 post("  Alpha: " + bandPower.alpha[activeChannel] + "\n");
 post("  Beta: " + bandPower.beta[activeChannel] + "\n");
 post("  Gamma: " + bandPower.gamma[activeChannel] + "\n");
 post(
  "Buffer Size: " + eegBuffer[activeChannel].length + "/" + bufferSize + "\n"
 );
 post("FFT Size: " + fftSize + "\n");
 post("Mapping Mode: " + mappingMode + "\n");
 post("Current State: " + currentState + "\n");
 post("---------------------------\n");
}

// Force data processing with test data
function testProcess() {
 post("Testing band power processing with sample data\n");

 // Simulate band power data
 processBandsData("delta", 0.5);
 processBandsData("theta", 0.2);
 processBandsData("alpha", 0.3);
 processBandsData("beta", 0.15);
 processBandsData("gamma", 0.1);

 // Output info
 debug();
}

// Function to test all outputs
function testOutputs() {
 post("Testing all outputs\n");

 // Test MIDI output
 outlet(0, ["control", 1, 20, 64]); // MIDI CC

 // Create array data for FFT instead of jitter matrix
 var fftData = [];
 for (var i = 0; i < 64; i++) {
  fftData.push(Math.sin(i / 5) * 0.5 + 0.5);
 }
 outlet(1, ["fft_data"].concat(fftData));

 // Test band power output
 outlet(2, ["bandpower", 0, 0.5, 0.2, 0.3, 0.15, 0.1]);

 // Test state output
 outlet(3, ["state", "testing"]);

 // Test raw output - MUST use "rawdata" not "raw"
 var rawData = [];
 for (var i = 0; i < 8; i++) {
  rawData.push(Math.sin(i / 2) * 100);
 }
 outlet(4, ["rawdata"].concat(rawData));

 post("All outputs tested\n");
}

function testMidi() {
 // Send test MIDI values to all MidiVolve parameters
 outlet(0, ["control", 1, 30, 64]); // Variation
 outlet(0, ["control", 1, 31, 96]); // Pattern Length
 outlet(0, ["control", 1, 32, 32]); // Rhythm Complexity
 outlet(0, ["control", 1, 33, 16]); // Scale Selection
 outlet(0, ["control", 1, 34, 48]); // Randomization

 post("Test MIDI values sent\n");
}

function testMidiRange() {
 post("Testing MIDI value range and formatting\n");

 // Test each band with extreme high values
 bandPower.delta[activeChannel] = 9999; // Deliberately extreme
 bandPower.theta[activeChannel] = 5000;
 bandPower.alpha[activeChannel] = 2000;
 bandPower.beta[activeChannel] = 1000;
 bandPower.gamma[activeChannel] = 500;

 // Check if values are properly limited
 sendBandPowerMidi(activeChannel);
 sendMidiVolveControls(activeChannel);

 post("Values should be limited to 0-127 range\n");
}

// Add this new function to test visualizations
function visualize() {
 post("Sending visualization test data...\n");

 // Generate test data for all outputs
 var channelsToShow = processAllChannels ? numChannels : 1;

 for (var ch = 0; ch < channelsToShow; ch++) {
  // 1. FFT data - different patterns for each channel
  var fftData = [];
  for (var i = 0; i < 64; i++) {
   // Create slightly different patterns for each channel
   fftData.push(
    Math.exp(-i / (10 + ch)) * Math.sin(i / (3 + ch * 0.5)) * 0.5 + 0.5
   );
  }
  outlet(1, ["fft_data", ch].concat(fftData));

  // 2. Band powers - different for each channel
  var scale = 1.0 - ch * 0.1;
  outlet(2, [
   "bandpower",
   ch,
   0.7 * scale,
   0.4 * scale,
   0.3 * scale,
   0.2 * scale,
   0.1 * scale,
  ]);

  // 3. Raw EEG - with phase shift for each channel
  var rawData = [];
  for (var i = 0; i < 100; i++) {
   rawData.push(Math.sin(i / 5 + (ch * Math.PI) / 4) * 100);
  }
  outlet(4, ["rawdata", ch].concat(rawData));
 }

 // 3. Mental state (common for all)
 outlet(3, ["state", "visualizing"]);

 post("Visualization test data sent for " + channelsToShow + " channels\n");
}
