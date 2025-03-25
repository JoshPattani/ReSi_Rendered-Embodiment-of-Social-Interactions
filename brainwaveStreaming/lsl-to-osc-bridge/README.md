# lsl-to-osc-bridge

## Overview

The lsl-to-osc-bridge project facilitates the real-time transmission of biometric data from LSL (Lab Streaming Layer) to OSC (Open Sound Control). This bridge allows the EEG data to be used with a wide range of applications that support OSC, such as Max/MSP, Ableton Live, and Pure Data.

## Features

- Real-time data streaming from LSL to OSC.
- Configurable settings for LSL and OSC connections.
- Example patches for Max/MSP and Ableton Live to demonstrate usage.
- Optional data processing capabilities to filter or transform data before sending.
- Feature extraction for EEG data including alpha, beta, delta, and theta band power, focus (mindfulness) and relation.

## Installation

1. Create & activate a virtual environment (if you haven't already):

   ```sh
   python -m venv venv
   source venv/bin/activate
   ```

2. Install the required dependencies:
   ```sh
   cd path\to\ReSi_Rendered-Embodiment-of-Social-Interactions\brainwaveStreaming\lsl-to-osc-bridge
   pip install -r requirements.txt
   ```
   Or install the required dependencies manually:
   ```sh
   pip install numpy pylsl python-osc scipy termcolor brainflow
   ```

## Configuration

Configuration settings can be adjusted in `src/config.py`. This file includes parameters such as IP addresses and ports for both LSL and OSC connections.

1. The bridge uses a configuration file to determine which LSL streams to monitor and where to send OSC messages.

2. Copy the example configuration and modify it for your setup:

   ```sh
   cp examples/default_config.py config/my_config.py
   ```

3. Edit `src/config.py` to set your desired parameters for LSL and OSC connections.

- OSC destination IP and port
- Any specific LSL streams you want to capture (optional)
- Data processing settings

## Usage

1. Make sure your OpenBCI boards are streaming via the dual streaming script
2. Run the bridge:

   ```
   cd path\to\ReSi_Rendered-Embodiment-of-Social-Interactions\brainwaveStreaming\lsl-to-osc-bridge
   python -m src.bridge
   ```

3. The bridge will:
   - Automatically discover the participant1 and participant2 LSL streams
   - Process the EEG data (filtering, band power calculation)
   - Send OSC messages to the configured destination
4. You'll see output indicating successful connections:
   ```
   Found 4 LSL streams!
   Matched stream: 'participant1_my_obci_EXG' of type 'EXG'
   Matched stream: 'participant1_my_obci_AUX' of type 'AUX'
   Matched stream: 'participant2_my_obci_EXG' of type 'EXG'
   Matched stream: 'participant2_my_obci_AUX' of type 'AUX'
   Bridging faster than the air-speed velocity of an unladen swallow...
   ```
5. While the bridge is running:
   - Press 'r' to reset the buffer
   - Press 'g' to check signal quality statistics
   - Press 'q' to quit

Ensure that your LSL source is active and that the OSC destination is correctly configured in the settings.

## OSC message addresses and data format

The bridge sends several types of OSC messages:

1. Raw EEG data:
   - Address: `/userA_my_obci_EXG/eeg/channels`
   - Data: Array of channel values
2. EEG band powers (Delta, Theta, Alpha, Beta, Gamma):

   - Address: `/userA_my_obci_EXG/eeg/bands/alpha`
   - Data: Normalized band power value

3. Focus and relaxation metrics:

   - Address: `/userA_my_obci_EXG/metrics/mindfulness`
   - Data: Mindfulness score (0-1)
   - Address: `/userA_my_obci_EXG/metrics/restfulness`
   - Data: Restfulness score (0-1)

4. Signal quality metrics:

   - Address: `/userA_my_obci_EXG/eeg/quality`
   - Data: Signal quality score (0-1)

The base address can be configured in `src/config.py`.

## Receiving OSC Messages

You can receive these OSC messages in software like:

- Max/MSP
- TouchDesigner
- Processing
- Pure Data
- Custom applications using OSC libraries

### Examples for receiving in Max/MSP and Ableton Live

- **Max/MSP Example**: Open `examples/max_msp_example.maxpat` to see how to receive OSC messages in Max/MSP.
- **Ableton Live Example**: Use `examples/ableton_example.amxd` to integrate OSC messages into your Ableton Live session.

## Troubleshooting

### No LSL Streams Found

- Make sure the OpenBCI streaming is running
- Check that the bridge is on the same network as the streaming computer
- Try setting `LSL_SOURCE_NAME` to `None` in your config to discover all streams

### OSC Messages Not Received

- Verify the OSC destination IP and port are correct
- Check that your OSC receiver is listening on the correct port
- Ensure no firewall is blocking the UDP communication
- Test with a simple OSC application like OSC Monitor

### High CPU Usage

- Reduce the sample rate in your OpenBCI configuration
- Increase the processing buffer size in the bridge
- Limit the number of processed channels

## Contributing

Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## Acknowledgments

This project was inspired by the [Brainflow_LSL](https://github.com/marles77/openbci-brainflow-lsl) project, which provides a powerful framework for streaming data from BCI devices over LSL.
