"""
LSL to OSC Bridge
=================

Author: Josh Pattani https://github.com/JoshPattani
Date: 2025-03-25

This script bridges data from an LSL stream to an OSC server. It receives EEG data from an LSL stream, processes the data, and sends the results via OSC to a specified IP address and port. The script was designed to work with OpenBCI Cyton boards and the Brainflow-LSL streaming program by Marcin Lesniak @marles77
"""

import time
import numpy as np

# from termcolor_dg import colored, cprint
from rich import print as rprint
from rich import inspect  # Switched to rich for beautiful console output
from rich.console import Console
from pyfiglet import Figlet
import threading
from lsl_receiver import LSLReceiver
from osc_sender import OSCSender
import config
from utils.data_processor import process_data, calculate_band_powers, check_eeg_quality
import os

os.system("color")

# Enable BrainFlow logging for debugging
from brainflow.board_shim import BoardShim, LogLevels
from brainflow.data_filter import DataFilter, DetrendOperations

BoardShim.enable_board_logger()
DataFilter.enable_data_logger()
DataFilter.set_log_level(LogLevels.LEVEL_INFO.value)

# Also add more buffer accumulation
buffer_size = 675  # Adjust based on your sampling rate - aim for 2+ seconds
samples_buffer = {"data": [], "count": 0}
sample_buffer = {}
last_focus = 0.5
last_relax = 0.5
smooth_factor = 0.85  # Higher = more smoothing
INITIALIZED = False


def key_listener():
    """Listen for key presses to control the bridge"""
    while True:
        try:
            cmd = input().strip().lower()
            if cmd == "r":
                reset_buffer()
            elif cmd == "q":
                print("Stopping streams...")
                break
        except Exception:
            pass


# Start the key listener thread
key_thread = threading.Thread(target=key_listener)
key_thread.daemon = True
key_thread.start()


def hello_world():
    s = Figlet(font="isometric1")
    f = Figlet(font="smslant", width=200)
    rprint(f"[magenta]{s.renderText(' RESI')}[/magenta]")
    rprint(f"[cyan]{f.renderText('LSL-to-OSC')}[/cyan]")
    rprint(f"[cyan]{s.renderText(' Bridger!')}[/cyan]")
    print("\n")


def main():
    global INITIALIZED
    if not INITIALIZED:
        hello_world()
        INITIALIZED = True

    # Set LSL source name and data type to None for this session to find any streams
    # This overrides config.py settings to ensure we can find OpenBCI streams
    lsl_source_name = config.LSL_SOURCE_NAME
    lsl_data_type = config.LSL_DATA_TYPE

    # Try first with specific stream names if provided in config
    attempt = 1
    max_attempts = 3
    lsl_receiver = None

    while attempt <= max_attempts:
        print(f"Attempt {attempt}/{max_attempts} to find LSL streams...")

        # On first attempt, use config settings if available
        # On subsequent attempts, use more permissive settings
        if attempt == 1:
            search_name = lsl_source_name
            search_type = lsl_data_type
        else:
            print("Trying with broader search criteria...")
            search_name = None  # Accept any stream name
            search_type = (
                ["EEG", "EXG"] if attempt == 2 else None
            )  # Accept any stream type on final attempt

        # Initialize LSL Receiver with current search parameters
        lsl_receiver = LSLReceiver(search_name, search_type)

        # Start receiving LSL streams
        print(f"Searching for streams with name: {search_name} and type: {search_type}")
        lsl_receiver.start_streams()

        # If we found streams, break out of the loop
        if lsl_receiver.inlets:
            rprint(f"[green]Found {len(lsl_receiver.inlets)} LSL streams![/green]")
            break

        # Clean up and try again
        lsl_receiver.stop_streams()
        lsl_receiver = None
        attempt += 1

        # Wait before next attempt
        rprint("[yellow]Waiting before next attempt...[/yellow]")
        time.sleep(2)

    if not lsl_receiver or not lsl_receiver.inlets:
        rprint(
            "[red]Failed to find any LSL streams after multiple attempts. Exiting.[/red]"
        )
        return

    rprint(
        "[blue]Bridging faster than the air-speed velocity of an unladen swallow...[/blue]",
    )
    rprint(
        "[yellow]Press 'r' to reset the buffer, 'g' to check railed percentages, or 'q' to quit.[/yellow]",
    )

    # Initialize OSC Sender
    osc_sender = OSCSender(config.OSC_IP, config.OSC_PORT)

    try:
        while True:
            # Receive data from LSL
            data = lsl_receiver.get_data()
            if data:
                for stream_id, stream_data in data.items():
                    if stream_data["type"] in ["EEG", "EXG"]:
                        samples = np.array(
                            stream_data["samples"]
                        )  # Convert to numpy array
                        sampling_rate = stream_data.get(
                            "sample_rate", 250
                        )  # Default or get from stream

                        # Skip processing if no samples
                        if samples.size == 0:
                            print("No samples received, skipping processing")
                            continue

                        print(
                            f"Processing {samples.shape[0]} samples for {samples.shape[1]} channels"
                        )

                        # Buffer management - store at least 2 seconds of data
                        stream_id = stream_data["name"]
                        if stream_id not in sample_buffer:
                            sample_buffer[stream_id] = []

                        # Check if we have new samples
                        if samples.size > 0:
                            print(f"Adding {samples.shape[0]} new samples to buffer")

                            # Add new samples to buffer
                            sample_buffer[stream_id].extend(samples.tolist())

                            # Keep only the most recent 2.5 seconds (with some overlap)
                            max_buffer_size = int(sampling_rate * 2.5)
                            if len(sample_buffer[stream_id]) > max_buffer_size:
                                sample_buffer[stream_id] = sample_buffer[stream_id][
                                    -max_buffer_size:
                                ]

                            # Only process if we have enough data
                            if len(sample_buffer[stream_id]) >= int(sampling_rate * 2):
                                # Convert to numpy array with proper shape
                                buffered_samples = np.array(sample_buffer[stream_id])

                                # Add timestamp to help track changes
                                current_time = time.time()

                                # Process all active channels together
                                active_channels = []
                                for ch in range(buffered_samples.shape[1]):
                                    # Check if channel has any non-zero values
                                    if not np.all(buffered_samples[:, ch] == 0):
                                        active_channels.append(ch)

                                if len(active_channels) == 0:
                                    print(
                                        "No active channels detected, skipping processing"
                                    )
                                    continue

                                print(f"Active channels: {active_channels}")

                                # Extract only active channels
                                active_data = buffered_samples[:, active_channels]

                                # Send raw EEG data via OSC
                                if len(active_channels) > 0:
                                    # Get the latest sample (last row of the buffer)
                                    latest_sample = active_data[-1]

                                    # Create OSC message with raw EEG data
                                    raw_osc_message = {
                                        "type": "EEG_Raw",
                                        "name": stream_data["name"],
                                        "channels": latest_sample.tolist(),
                                        "timestamp": current_time,
                                    }

                                    # Send raw EEG data via OSC
                                    osc_sender.send_message(raw_osc_message)
                                    print(
                                        f"Sent raw EEG data for {len(active_channels)} channels"
                                    )

                                # Force buffer reset (sliding window approach)
                                # Keep only the most recent 20% of samples to force updates
                                retain_samples = int(
                                    len(sample_buffer[stream_id]) * 0.8
                                )
                                sample_buffer[stream_id] = sample_buffer[stream_id][
                                    -retain_samples:
                                ]

                                # Perform Railed calculations for each active channels
                                try:
                                    railed_percentages = {}
                                    colors = [
                                        "bright_black",
                                        "purple3",
                                        "navy_blue",
                                        "dark_green",
                                        "gold3",
                                        "dark_orange",
                                        "red3",
                                    ]
                                    for ch in active_channels:
                                        railed_percentage = (
                                            DataFilter.get_railed_percentage(
                                                active_data[:, ch], gain=config.GAIN
                                            )
                                        )
                                        railed_percentages[ch] = railed_percentage
                                        color = colors[ch % len(colors)]
                                        rprint(
                                            f"[{color}]Channel {ch} railed percentage: {railed_percentage}[/{color}]"
                                        )
                                except Exception as e:
                                    rprint(
                                        f"[bright_red]Error calculating railed percentages: {e}[/bright_red]"
                                    )

                                try:
                                    # Normalize the data before processing
                                    # This helps ensure consistent results
                                    data_norm = np.zeros_like(active_data)
                                    for ch_idx in range(active_data.shape[1]):
                                        # Get channel data
                                        ch_data = active_data[:, ch_idx]

                                        # Create a copy for detrending
                                        ch_detrended = ch_data.copy()

                                        # Apply detrend in-place (this modifies ch_detrended directly)
                                        DataFilter.detrend(
                                            ch_detrended, DetrendOperations.LINEAR.value
                                        )

                                        # Standardize to have zero mean, unit variance
                                        ch_mean = np.mean(ch_detrended)
                                        ch_std = np.std(ch_detrended)
                                        if ch_std > 0:
                                            data_norm[:, ch_idx] = (
                                                ch_detrended - ch_mean
                                            ) / ch_std
                                        else:
                                            data_norm[:, ch_idx] = (
                                                ch_detrended - ch_mean
                                            )

                                    # Use the normalized data for processing
                                    processed_data, extras = process_data(
                                        data_norm,  # Use normalized data
                                        eeg_channels=active_channels,
                                        scale_factor=config.SCALE_FACTOR,
                                        calculate_bands=True,
                                        calculate_metrics=True,
                                        sampling_rate=sampling_rate,
                                    )

                                    # Access average band powers and metrics
                                    metrics = extras.get(
                                        "metrics", {}
                                    )  # Metrics for the entire data
                                    band_powers = extras.get(
                                        "band_powers", {}
                                    )  # Average band powers across all channels

                                    # Anti-jitter filter commented out for tomorrow's test 3/21/2025, might need to adjust. Might be represented in the metrics meaningfully.

                                    # global last_focus, last_relax
                                    # if "mindfulness" in metrics and isinstance(
                                    #     metrics["mindfulness"], (int, float)
                                    # ):
                                    #     metrics["mindfulness"] = (
                                    #         smooth_factor * last_focus
                                    #         + (1 - smooth_factor)
                                    #         * metrics["mindfulness"]
                                    #     )
                                    #     last_focus = metrics["mindfulness"]

                                    # if "restfulness" in metrics and isinstance(
                                    #     metrics["restfulness"], (int, float)
                                    # ):
                                    #     metrics["restfulness"] = (
                                    #         smooth_factor * last_relax
                                    #         + (1 - smooth_factor)
                                    #         * metrics["restfulness"]
                                    #     )
                                    #     last_relax = metrics["restfulness"]

                                    # # Safety check before sending
                                    # for key in ["mindfulness", "restfulness"]:
                                    #     if key in metrics and not isinstance(
                                    #         metrics[key], (int, float)
                                    #     ):
                                    #         metrics[key] = 0.0

                                    # Send metrics data
                                    osc_message = {
                                        "type": "EEG_Metrics",
                                        "name": stream_data["name"],
                                        "metrics": metrics,
                                        "bands": band_powers,
                                    }

                                    osc_sender.send_message(osc_message)
                                    rprint(
                                        f"[chartreuse3]Focus: {metrics.get('mindfulness', 0)}[/chartreuse3], [light_slate_blue]Relaxation: {metrics.get('restfulness', 0)}[/light_slate_blue]"
                                    )

                                except Exception as e:
                                    rprint(
                                        f"[bright_red]Error processing EEG data: {e}[/bright_red]"
                                    )
                                    import traceback

                                    traceback.print_exc()

                                rprint(
                                    f"[dark_cyan]Processed {len(active_channels)} active channels: {active_channels}[/dark_cyan]"
                                )

                # Send raw data as well if needed
                # osc_sender.send_message(data)
                print(
                    f"Success: Sent OSC Message to {config.OSC_IP} on port {config.OSC_PORT}"
                )

            # time.sleep(config.DATA_SEND_INTERVAL)
    except KeyboardInterrupt:
        rprint("[orange3]Stopping the bridge...[/orange3]")
    finally:
        lsl_receiver.stop_streams()

    rprint("[dark_cyan]Bridge has been stopped.[/dark_cyan]")


def reset_buffer():
    """Force a reset of the data buffer"""
    global sample_buffer
    sample_buffer = {}
    rprint("[yellow]Buffer reset![/yellow]")


if __name__ == "__main__":
    main()
