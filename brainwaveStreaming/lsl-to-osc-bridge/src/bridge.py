import time
from lsl_receiver import LSLReceiver
from osc_sender import OSCSender
import config


def main():
    # Initialize LSL Receiver and OSC Sender
    lsl_receiver = LSLReceiver(config.LSL_SOURCE_NAME, config.LSL_DATA_TYPE)
    osc_sender = OSCSender(config.OSC_IP, config.OSC_PORT)

    # Start receiving LSL streams
    lsl_receiver.start_streams()

    try:
        while True:
            # Receive data from LSL
            data = lsl_receiver.get_data()
            if data:
                # Debug print
                print("Received LSL Data")
                # Send data as OSC message
                osc_sender.send_message(data)
                # Debug print
                print(
                    f"Success: Sent OSC Message to {config.OSC_IP} on port {config.OSC_PORT}"
                )
            time.sleep(config.DATA_SEND_INTERVAL)
    except KeyboardInterrupt:
        print("Stopping the bridge...")
    finally:
        lsl_receiver.stop_streams()

    print("Bridge has been stopped.")


if __name__ == "__main__":
    main()
