# Description: Configuration file for the LSL to OSC bridge

LSL_SOURCE_NAME = "obci_cyton"  # Name of the LSL stream to connect to
LSL_DATA_TYPE = "eeg"  # Type of data to receive from the LSL stream

"""
IP addresses

Home network:
- My computer:
    - "192.168.0.8"

- Cass's Home WiFI network:
    - 10.0.0.197 - Cass's IP Address (Visual computer)
    - 10.0.0.7 - Jaz's IP Address (Audio computer)
    - 10.0.0.155 - My IP Address 

UCB network:
- My computer:
    - "10.201.78.68"
- Cass's computer:
    - "10.201.6.0"
- Jaz's computer:
    - "10.201.65.186"
  
BLACKBOX PERFORMANCE STUDIO

UCB Wireless network:
- Audio computer:
    -
- Visual computer:
    -

BBX network:
- Audio computer:
    - "192.168.1.105"
    - "192.168.1.154"
- Visual computer:
    - "192.168.1.170"
"""

# Multiple destination IP addresses for audio and visual computers
OSC_IP = [
    "192.168.1.170",
    "192.168.1.154",
    "192.168.1.32",
]  # [Audio Computer, Visual Computer, my comp]
OSC_PORT = 42069  # Same port for both destinations
OSC_ADDRESS = "/obci"  # OSC address to send data to

DATA_SEND_INTERVAL = 0.001  # Interval in seconds for sending data
SCALE_FACTOR = 1  # Scaling factor for the data
SAMPLE_RATE = 250  # Sampling rate of the data
GAIN = 24  # Gain setting of the OpenBCI Cyton board
DEBUG_MODE = False  # Set to False in production
IGNORE_QUALITY_CHECKS = False  # Set to False in production

# OSC Discovery Configuration
ENABLE_DISCOVERY = True  # Set to False to disable discovery
OSC_DISCOVERY_PORT = 8080  # Port for discovery messages (same as OSC_PORT)
