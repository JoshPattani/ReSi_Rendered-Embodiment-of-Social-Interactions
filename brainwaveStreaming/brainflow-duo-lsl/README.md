# Dual OpenBCI Streaming with BrainFlow and LSL

This component allows you to stream data from two OpenBCI Cyton boards simultaneously using BrainFlow and Lab Streaming Layer (LSL). Each participant's data is clearly identified with unique stream names.

## Hardware Setup

1. OpenBCI Cyton Boards: You need two Cyton boards, each with its own USB dongle
2. USB Ports: Connect each dongle to a separate USB port on your computer\*
3. Electrodes: Place electrodes according to your experimental protocol

\*Important: Before running the script, you must first use the OpenBCI GUI to set up each Cyton board individually using the manual connection configuration. This step is necessary to ensure that each board is identified to the correct COM port.

## Configuration

1. Create two separate YAML configuration files for each Cyton board (use the provided examples as templates):

   - cyton_1.yml - Configuration for Participant 1
   - cyton_2.yml - Configuration for Participant 2

2. Modify the YAML files to set:

   - Serial ports for each board (e.g., COM12, COM19)
   - Channel names and configurations
   - Any other board-specific settings

Example configuration (cyton_1.yml):

```yaml
args:
 timeout: 0
 ip_address: 127.0.0.1
 ip_port: 6677
 board_id: 2
 master_board: 0
 serial_port: COM12
 name: my_obci
 data_type:
  - EXG
  - AUX
 channel_names:
  EXG: Fp1,Fp2,Fpz,F7,F8,O1,O2,Oz
  AUX: P11,P12,P17
 uid: brainflow
 daisy_attached: no
 max_time: 7200
 delay: 0
commands:
 chan1: x1060110Xx2060110Xx3060110Xx4060110Xx5060110Xx6060110Xx7060110Xx8060110X
```

## Dependencies

Create a new Python environment and install the required dependencies:

```sh
# macOS/Linux
# You may need to run `sudo apt-get install python3-venv` first on Debian-based OSs
python3 -m venv .venv

# Windows
# You can also use `py -3 -m venv .venv`
python -m venv .venv
```

Activate the environment:

```sh
# macOS/Linux
source .venv/bin/activate

# Windows
.venv\Scripts\activate
```

Install the required Python packages:

```sh
pip install --upgrade numpy brainflow pylsl pyserial PyYAML
```

Helpful tip: If you're using Visual Studio Code, you can select the Python interpreter for your environment by clicking on the Python version in the bottom left corner.

- [Python Environment Setup](https://code.visualstudio.com/docs/python/environments#_creating-environments)

## Usage

1. Open a command prompt and navigate to the brainflow-duo-lsl directory:

   ```sh
   cd path\to\ReSi_Rendered-Embodiment-of-Social-Interactions\brainwaveStreaming\brainflow-duo-lsl
   ```

2. Run the dual streaming script with your configuration files:

   ```sh
   python obci_brainflow_lsl_duo_novo.py --set cyton_1.yml cyton_2.yml
   ```

3. Follow the prompts in the console:

   - When asked to "Initiate", type `y` and press Enter
   - When asked to "Send commands to board", type `y` and press Enter
   - When asked to "Start streaming", type `y` and press Enter

4. The script will create two sets of LSL streams, one for each participant:

   - `participant1_my_obci_EXG` - EEG data from participant 1
   - `participant1_my_obci_AUX` - Accelerometer data from participant 1
   - `participant2_my_obci_EXG` - EEG data from participant 2
   - `participant2_my_obci_AUX` - Accelerometer data from participant 2

5. To stop streaming, press `q` and Enter when prompted.

## Viewing Available LSL Streams

You can verify your streams are working by using an LSL stream viewer:

```py
python -m pylsl.examples.ReceiveAndPlot
```

## Troubleshooting

### Board Connection Issues

- Ensure each board has a unique serial port
- Check your serial port names in Device Manager (Windows) or with `ls /dev/tty*` (macOS/Linux)
- Try unplugging and replugging the dongles
- Make sure your boards are powered on and switched to 'PC' mode

### Stream Not Appearing

- Check that the boards are properly paired with their dongles
- Verify the serial ports in your YAML files match the actual ports
- Make sure you've started the streaming by typing 'y' at all prompts

### Poor Signal Quality

- Check electrode placement and ensure good contact with the skin
- Verify channel settings in your YAML configuration files
- Use the OpenBCI GUI to test signal quality before streaming

## Next Steps

After starting the dual streams, you'll typically want to:

1. Start the [LSL-to-OSC Bridge](/brainwaveStreaming/lsl-to-osc-bridge/README.md) to convert the data

But you can also:

1. Monitor the data using [BrainVision LSL Viewer](https://www.brainproducts.com/downloads/more-software/#46df73ca21023a5a0)
2. Record the data using [Lab Recorder](https://github.com/labstreaminglayer/App-LabRecorder)
