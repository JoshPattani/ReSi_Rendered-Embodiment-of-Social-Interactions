from pylsl import StreamInlet, resolve_streams
import threading
import time
import numpy as np


class LSLReceiver:
    """
    Receives data from LSL streams and makes it available for the OSC bridge.
    Can handle multiple streams and different data types (EEG, markers, etc.).
    """

    def __init__(self, source_names=None, stream_types=None):
        """
        Initialize LSL receiver.

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
        self.inlets = []
        self.stream_info = []
        self.data_buffers = {}  # Dictionary to store latest data for each stream
        self.running = False
        self.thread = None
        self.lock = threading.Lock()  # For thread-safe access to data_buffers

    def find_streams(self):
        """Find all LSL streams matching the specified criteria."""
        print("Looking for LSL streams...")

        # Prepare the result list
        streams = []

        # If no specific criteria, get all available streams
        if not self.source_names and not self.stream_types:
            streams = resolve_streams()
        else:
            # Check for streams matching source names
            if self.source_names:
                for name in self.source_names:
                    matching_streams = resolve_streams()
                    if matching_streams:
                        streams.extend(matching_streams)

            # Check for streams matching types
            if self.stream_types:
                for stream_type in self.stream_types:
                    matching_streams = resolve_streams()
                    if matching_streams:
                        streams.extend(matching_streams)

        # Remove duplicates
        unique_streams = []
        unique_ids = set()
        for stream in streams:
            if stream.uid() not in unique_ids:
                unique_streams.append(stream)
                unique_ids.add(stream.uid())

        return unique_streams

    def start_streams(self):
        """Start receiving data from LSL streams."""
        if self.running:
            print("LSL receiver is already running.")
            return

        # Find streams
        streams = self.find_streams()
        if not streams:
            print("No LSL streams found. Make sure your devices are streaming.")
            return

        # Create inlets for each stream
        for stream in streams:
            print(
                f"Found stream: {stream.name()} - {stream.type()} ({stream.channel_count()} channels at {stream.nominal_srate()} Hz)"
            )
            inlet = StreamInlet(stream)
            self.inlets.append(inlet)
            self.stream_info.append(
                {
                    "name": stream.name(),
                    "type": stream.type(),
                    "channel_count": stream.channel_count(),
                    "sample_rate": stream.nominal_srate(),
                    "uid": stream.uid(),
                }
            )
            # Initialize buffer for this stream
            self.data_buffers[stream.uid()] = {
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
        """Background thread that continuously receives data from LSL streams."""
        while self.running:
            for i, inlet in enumerate(self.inlets):
                stream_id = self.stream_info[i]["uid"]

                # Pull data chunk from the stream
                # Use a timeout of 0 for non-blocking
                try:
                    # For faster streams (e.g. EEG), pull multiple samples at once
                    if self.stream_info[i]["sample_rate"] > 100:
                        samples, timestamps = inlet.pull_chunk(max_samples=32)
                    else:
                        # For marker streams, pull one sample at a time
                        sample, timestamp = inlet.pull_sample(timeout=0)
                        samples = [sample] if sample else []
                        timestamps = [timestamp] if timestamp else []

                    if samples:
                        with self.lock:
                            # Store new data in buffer
                            self.data_buffers[stream_id]["samples"].extend(samples)
                            self.data_buffers[stream_id]["timestamps"].extend(
                                timestamps
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
