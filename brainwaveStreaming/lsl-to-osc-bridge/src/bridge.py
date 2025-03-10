import time
from lsl_receiver import LSLReceiver
from osc_sender import OSCSender
import config
from utils.data_processor import process_data, calculate_band_powers


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
                for stream_id, stream_data in data.items():
                    if stream_data["type"] in ["EEG", "EXG"]:
                        samples = stream_data["samples"]
                        sampling_rate = stream_data.get(
                            "channel_count", 250
                        )  # Default or get from stream

                        # Process each channel
                        for channel_idx in range(len(samples[0])):
                            channel_data = [sample[channel_idx] for sample in samples]

                            # Calculate bands and metrics
                            processed_data, extras = process_data(
                                channel_data,
                                calculate_bands=True,
                                calculate_metrics=True,  # Now we can calculate metrics too
                                sampling_rate=sampling_rate,
                            )

                            # Access band powers and metrics
                            band_powers = extras.get("band_powers", {})
                            metrics = extras.get("metrics", {})

                            # Print focus and relaxation scores
                            print(
                                f"Channel {channel_idx} - Focus score: {metrics.get('mindfulness', 0):.2f}, "
                                f"Relaxation score: {metrics.get('restfulness', 0):.2f}"
                            )

                            # Send both as OSC messages
                            osc_sender.send_message(
                                {
                                    f"channel_{channel_idx}_bands": band_powers,
                                    f"channel_{channel_idx}_metrics": metrics,
                                }
                            )

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
