# lsl-to-osc-bridge

## Overview

The lsl-to-osc-bridge project facilitates the real-time transmission of biometric data from LSL (Lab Streaming Layer) to OSC (Open Sound Control). This bridge allows for seamless integration of biometric data into audio-visual applications, enabling interactive experiences in environments such as Max/MSP and Ableton Live.

## Features

- Real-time data streaming from LSL to OSC.
- Configurable settings for LSL and OSC connections.
- Example patches for Max/MSP and Ableton Live to demonstrate usage.
- Optional data processing capabilities to filter or transform data before sending.
- Feature extraction for EEG data including alpha, beta, delta, and theta band power, focus (mindfulness) and relation.

## Installation

1. Clone the repository:

   ```
   git clone https://github.com/JoshPattani/ReSi_Rendered-Embodiment-of-Social-Interactions/lsl-to-osc-bridge.git
   cd lsl-to-osc-bridge
   ```

2. Create a virtual environment (optional but recommended):

   ```
   python -m venv venv
   source venv/bin/activate
   ```

3. Install the required dependencies:
   ```
   pip install -r requirements.txt
   ```

## Configuration

Configuration settings can be adjusted in `src/config.py`. This file includes parameters such as IP addresses and ports for both LSL and OSC connections.

## Usage

To run the bridge, execute the main script:

```
python src/bridge.py
```

Ensure that your LSL source is active and that the OSC destination is correctly configured in the settings.

## Examples

- **Max/MSP Example**: Open `examples/max_msp_example.maxpat` to see how to receive OSC messages in Max/MSP.
- **Ableton Live Example**: Use `examples/ableton_example.amxd` to integrate OSC messages into your Ableton Live session.

## Contributing

Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.
