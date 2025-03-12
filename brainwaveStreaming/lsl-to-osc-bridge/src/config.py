# Description: Configuration file for the LSL to OSC bridge

LSL_SOURCE_NAME = "YourLSLSourceName"
LSL_DATA_TYPE = "YourDataType"
OSC_IP = "10.201.78.68"  # UCB # local: "127.0.0.1"
OSC_PORT = 42069
OSC_ADDRESS = "/your/osc/address"
DATA_SEND_INTERVAL = 0.01  # Interval in seconds for sending data
SCALE_FACTOR = 1.0  # Scale factor for EEG data
SAMPLE_RATE = 250  # Sampling rate of the data
DEBUG_MODE = False  # Set to False in production
IGNORE_QUALITY_CHECKS = False  # Set to False in production
