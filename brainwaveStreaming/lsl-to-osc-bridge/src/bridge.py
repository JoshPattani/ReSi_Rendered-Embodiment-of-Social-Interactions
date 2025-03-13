import time
import numpy as np
from lsl_receiver import LSLReceiver
from osc_sender import OSCSender
import config
from utils.data_processor import process_data, calculate_band_powers, check_eeg_quality

# Enable BrainFlow logging for debugging
from brainflow.board_shim import BoardShim, LogLevels
from brainflow.data_filter import DataFilter

BoardShim.enable_board_logger()
DataFilter.enable_data_logger()
DataFilter.set_log_level(LogLevels.LEVEL_INFO.value)

# Also add more buffer accumulation
buffer_size = 500  # Adjust based on your sampling rate - aim for 2+ seconds
samples_buffer = {"data": [], "count": 0}


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

                        active_channels = []

                        # Process each channel
                        for channel_idx in range(len(samples[0])):
                            # Extract data for this specific channel
                            channel_data = [sample[channel_idx] for sample in samples]

                            # Skip completely flat channels
                            if np.min(channel_data) == np.max(channel_data) == 0:
                                print(f"Channel {channel_idx} - Inactive/off")
                                continue

                            # Check data quality for active channels only
                            if config.DEBUG_MODE and config.IGNORE_QUALITY_CHECKS:
                                data_quality = {
                                    "quality": "good"
                                }  # Skip quality checks in debug mode
                            else:
                                data_quality = check_eeg_quality(channel_data)
                            if data_quality["quality"] == "good":
                                # Accumulate samples in buffer
                                if len(samples_buffer["data"]) < buffer_size:
                                    samples_buffer["data"].extend(channel_data)
                                    samples_buffer["count"] += 1
                                else:
                                    # Process with the accumulated buffer
                                    processed_data, extras = process_data(
                                        samples_buffer["data"][
                                            -buffer_size:
                                        ],  # Use last buffer_size samples
                                        scale_factor=config.SCALE_FACTOR,
                                        calculate_bands=True,
                                        calculate_metrics=True,
                                        sampling_rate=sampling_rate,
                                    )
                                    # Reset buffer
                                    samples_buffer["data"] = []
                                    samples_buffer["count"] = 0

                                    # Access band powers and metrics
                                    band_powers = extras.get("band_powers", {})
                                    metrics = extras.get("metrics", {})
                                    # Print focus and relaxation scores
                                    print(
                                        f"Channel {channel_idx} - Focus: {np.array2string(metrics.get('mindfulness', 0), precision=2)}, "
                                        f"Relaxation: {np.array2string(metrics.get('restfulness', 0), precision=2)}"
                                    )

                                    # Send individual channel data
                                    osc_sender.send_message(
                                        {
                                            "type": "EEG_Metrics",  # Add missing type field
                                            "name": f"{stream_data['name']}_ch{channel_idx}",
                                            "bands": band_powers,
                                            "metrics": metrics,
                                        }
                                    )
                                    active_channels.append(channel_idx)
                            else:
                                print(
                                    f"Channel {channel_idx} - Poor quality: {data_quality.get('flatline', False)}"
                                )

                        print(
                            f"Processed {len(active_channels)} active channels: {active_channels}"
                        )

                # Send raw data as well if needed
                # osc_sender.send_message(data)
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
