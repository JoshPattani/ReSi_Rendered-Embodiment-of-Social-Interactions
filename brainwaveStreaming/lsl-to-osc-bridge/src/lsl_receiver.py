from mne_lsl.lsl import StreamInlet, resolve_streams, local_clock
import threading
import time
import numpy as np


class LSLReceiver:
    """
    Receives data from LSL streams and makes it available for the OSC bridge.
    Can handle multiple streams and different data types (EEG, markers, etc.).
    Uses MNE-LSL library for improved handling of EEG streams.
    """

    def __init__(self, source_names=None, stream_types=None):
        """
        Initialize LSL receiver using MNE-LSL.

        Args:
            source_names: List of stream source names to look for (e.g. ['OpenBCI_EXG', 'OpenBCI_AUX'])
                          If None, will connect to all available streams
            stream_types: List of stream types to look for (e.g. ['EEG', 'Markers'])
                          If None, will connect to all available streams
        """
        self.source_names = (
            source_names
            if isinstance(source_names, list)
            else [source_names] if source_names else None
        )
        self.stream_types = (
            stream_types
            if isinstance(stream_types, list)
            else [stream_types] if stream_types else None
        )
        self.inlets = []  # MNE-LSL StreamInlet objects
        self.stream_info = []
        self.data_buffers = {}  # Dictionary to store latest data for each stream
        self.running = False
        self.thread = None
        self.lock = threading.Lock()  # For thread-safe access to data_buffers

    def find_streams(self):
        """Find all LSL streams matching the specified criteria using MNE-LSL."""
        print("Looking for LSL streams...")

        # Get all available LSL streams using resolve_streams()
        all_streams = resolve_streams()

        if not all_streams:
            print("No LSL streams found.")
            return []

        # Print all available streams for debugging
        print(f"Found {len(all_streams)} LSL streams:")
        for i, stream in enumerate(all_streams):
            print(
                f"  Stream {i+1}: name='{stream.name}', type='{stream.stype}', channels={stream.n_channels}"
            )

        # Filter streams based on criteria
        matching_streams = []

        # If no specific criteria, return all streams
        if not self.source_names and not self.stream_types:
            print("No filters applied - returning all streams")
            return all_streams

        for stream in all_streams:
            # Check if stream name matches any of the source names
            name_match = False
            if not self.source_names:
                name_match = True
            else:
                # Try to match partial names - OpenBCI streams often have serial port added to name
                for source_name in self.source_names:
                    if source_name in stream.name:
                        name_match = True
                        break

            # Check if stream type matches any of the stream types
            type_match = not self.stream_types or stream.stype in self.stream_types

            if name_match and type_match:
                matching_streams.append(stream)
                print(f"  Matched stream: '{stream.name}' of type '{stream.stype}'")

        return matching_streams

    def start_streams(self):
        """Start receiving data from LSL streams using MNE-LSL."""
        if self.running:
            print("LSL receiver is already running.")
            return

        # Find streams
        streams = self.find_streams()
        if not streams:
            print(
                "No matching LSL streams found. Make sure your devices are streaming."
            )
            return

        # Create inlets for each stream
        for stream in streams:
            print(
                f"Found stream: {stream.name} - {stream.stype} ({stream.n_channels} channels at {stream.sfreq} Hz)"
            )

            # Create a StreamInlet for this stream
            inlet = StreamInlet(stream)
            # Open the stream to establish the connection
            inlet.open_stream()
            self.inlets.append(inlet)

            # Generate a unique ID for the stream
            stream_uid = f"{stream.name}_{stream.source_id}"

            self.stream_info.append(
                {
                    "name": stream.name,
                    "type": stream.stype,
                    "channel_count": stream.n_channels,
                    "sample_rate": stream.sfreq,
                    "uid": stream_uid,
                }
            )

            # Initialize buffer for this stream
            self.data_buffers[stream_uid] = {
                "samples": [],
                "timestamps": [],
                "latest_update": 0,
            }

        # Start the receiver thread
        self.running = True
        self.thread = threading.Thread(target=self._receive_data)
        self.thread.daemon = True
        self.thread.start()

        print(f"LSL receiver started, listening to {len(self.inlets)} streams.")

    def _receive_data(self):
        """Background thread that continuously receives data from LSL streams using MNE-LSL."""
        while self.running:
            for i, inlet in enumerate(self.inlets):
                stream_id = self.stream_info[i]["uid"]

                try:
                    # Use pull_chunk to get multiple samples at once if available
                    # Set timeout=0 for non-blocking operation
                    samples, timestamps = inlet.pull_chunk(timeout=0, max_samples=32)

                    if samples.size > 0:
                        with self.lock:
                            # Store new data in buffer
                            self.data_buffers[stream_id]["samples"].extend(
                                samples.tolist()
                            )
                            self.data_buffers[stream_id]["timestamps"].extend(
                                timestamps.tolist()
                            )

                            # Keep only the latest 1000 samples to avoid memory issues
                            if len(self.data_buffers[stream_id]["samples"]) > 1000:
                                self.data_buffers[stream_id]["samples"] = (
                                    self.data_buffers[stream_id]["samples"][-1000:]
                                )
                                self.data_buffers[stream_id]["timestamps"] = (
                                    self.data_buffers[stream_id]["timestamps"][-1000:]
                                )

                            self.data_buffers[stream_id]["latest_update"] = time.time()
                except Exception as e:
                    print(
                        f"Error receiving data from stream {self.stream_info[i]['name']}: {e}"
                    )

            # Small sleep to prevent CPU hogging
            time.sleep(0.001)

    def get_data(self):
        """
        Get the latest data from all streams.

        Returns:
            A dictionary with stream IDs as keys, each containing samples and timestamps
            that have not been processed yet.
        """
        if not self.running:
            return None

        result = {}
        with self.lock:
            for stream_id, buffer in self.data_buffers.items():
                # Return data only if there are new samples
                if buffer["samples"]:
                    # Find the stream info
                    info = next(
                        (info for info in self.stream_info if info["uid"] == stream_id),
                        None,
                    )

                    result[stream_id] = {
                        "name": info["name"],
                        "type": info["type"],
                        "channel_count": info["channel_count"],
                        "samples": buffer["samples"].copy(),
                        "timestamps": buffer["timestamps"].copy(),
                    }

                    # Clear the buffer after getting data
                    buffer["samples"] = []
                    buffer["timestamps"] = []

        return result if result else None

    def stop_streams(self):
        """Stop receiving data from LSL streams."""
        self.running = False
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2.0)

        # Close all inlets
        for inlet in self.inlets:
            inlet.close_stream()

        self.inlets = []
        self.stream_info = []
        self.data_buffers = {}
        print("LSL receiver stopped.")
