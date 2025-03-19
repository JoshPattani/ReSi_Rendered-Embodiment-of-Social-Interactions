# Description: Configuration file for the LSL to OSC bridge

LSL_SOURCE_NAME = "obci_cyton"  # Name of the LSL stream to connect to
LSL_DATA_TYPE = "eeg"  # Type of data to receive from the LSL stream
OSC_IP = "192.168.0.8"  # "10.201.78.68"  # UCB # local: "127.0.0.1"
OSC_PORT = 42069
OSC_ADDRESS = "/obci"  # OSC address to send data to
DATA_SEND_INTERVAL = 0.01  # Interval in seconds for sending data
SCALE_FACTOR = 1.0  # Scale factor for EEG data
SAMPLE_RATE = 250  # Sampling rate of the data
GAIN = 24  # Gain setting of the OpenBCI Cyton board
DEBUG_MODE = False  # Set to False in production
IGNORE_QUALITY_CHECKS = False  # Set to False in production
