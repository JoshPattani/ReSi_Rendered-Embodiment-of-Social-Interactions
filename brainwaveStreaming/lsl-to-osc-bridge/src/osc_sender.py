from pythonosc import udp_client
from pythonosc.osc_message_builder import OscMessageBuilder
import numpy as np
import json
import time


class OSCSender:
    """
    Sends data to OSC clients (like Max/MSP or Ableton).
    Formats LSL data appropriately for OSC protocol.
    """

    def __init__(self, ip="127.0.0.1", ports=None):
        """
        Initialize OSC sender.

        Args:
            ip: IP address(es) to send OSC messages to (default: localhost)
               Can be a single IP or a list of IPs for multiple destinations
            ports: Dictionary mapping stream types to ports or single port for all streams
                   Example: {'EEG': 9000, 'Markers': 9001} or just 9000
        """
        # Handle ip parameter as either a single IP or a list of IPs
        if isinstance(ip, list):
            self.ips = ip
        else:
            self.ips = [ip]  # Convert single IP to list

        # Setup ports - either a dict mapping stream types to ports or a single port for all
        if isinstance(ports, dict):
            self.ports = ports
        else:
            self.default_port = ports if ports else 9000
            self.ports = {}

        # Create clients dictionary - will be populated as needed
        self.clients = {}

        # Default setup for common stream types if not specified
        if isinstance(self.ports, dict):
            if "EEG" not in self.ports:
                self.ports["EEG"] = 9000
            if "Markers" not in self.ports:
                self.ports["Markers"] = 9001
            if "AUX" not in self.ports:
                self.ports["AUX"] = 9002

        # Create initial clients for predefined ports
        self._create_clients()

        # Stats for monitoring
        self.messages_sent = 0
        self.last_sent_time = 0

    def _create_clients(self):
        """Create OSC clients for all predefined ports."""
        if isinstance(self.ports, dict):
            for stream_type, port in self.ports.items():
                for ip in self.ips:
                    client_key = f"{ip}:{port}"
                    if client_key not in self.clients:
                        self.clients[client_key] = udp_client.SimpleUDPClient(ip, port)
        else:
            # Just create a client for the default port
            for ip in self.ips:
                client_key = f"{ip}:{self.default_port}"
                if client_key not in self.clients:
                    self.clients[client_key] = udp_client.SimpleUDPClient(
                        ip, self.default_port
                    )

    def _get_port_for_stream(self, stream_type):
        """Determine which port to use for a given stream type."""
        if isinstance(self.ports, dict) and stream_type in self.ports:
            return self.ports[stream_type]
        return self.default_port

    def _ensure_client_exists(self, port):
        """Make sure a client exists for the given port."""
        for ip in self.ips:
            client_key = f"{ip}:{port}"
            if client_key not in self.clients:
                self.clients[client_key] = udp_client.SimpleUDPClient(ip, port)

    def start(self):
        """Initialize the OSC sender."""
        print(f"OSC sender initialized. Ready to send data to {', '.join(self.ips)}")
        for port in (
            set([self._get_port_for_stream(k) for k in self.ports.keys()])
            if isinstance(self.ports, dict)
            else [self.default_port]
        ):
            print(f"- OSC port {port} configured")

    def _send_value(self, address, value, port):
        """Helper method to send different types of values"""
        # Handle different data types
        if isinstance(value, dict):
            # Handle nested dictionaries
            for sub_key, sub_value in value.items():
                sub_address = f"{address}/{sub_key}"
                self._send_value(sub_address, sub_value, port)
        elif isinstance(value, (list, np.ndarray)):
            # Handle arrays by sending each element with an index
            for i, item in enumerate(value):
                item_address = f"{address}/{i}"
                self._send_value(item_address, item, port)
        else:
            # Handle primitive types (convert to float if numeric)
            try:
                if isinstance(value, (int, float, np.number)):
                    value = float(value)
                elif value is None:
                    value = 0.0
                # Send to all clients for this port
                for ip in self.ips:
                    client_key = f"{ip}:{port}"
                    if client_key in self.clients:
                        self.clients[client_key].send_message(address, value)
            except Exception as e:
                print(
                    f"Error sending OSC message to {address}: {e} (value: {value}, type: {type(value)})"
                )
                # Fallback to string representation if conversion fails
                for ip in self.ips:
                    client_key = f"{ip}:{port}"
                    if client_key in self.clients:
                        self.clients[client_key].send_message(address, str(value))

    def send_message(self, data):
        """
        Send data via OSC to all configured IP addresses.

        Args:
            data: Dictionary containing data from LSLReceiver.get_data()
        """
        if not data:
            print("No data to send.")
            return

        # Check if this is a standard LSL data format or a custom message
        if "type" in data and isinstance(data["type"], str):
            # This appears to be a direct/custom message
            port = self._get_port_for_stream(data["type"])
            self._ensure_client_exists(port)

            # Handle EEG_Metrics formatted message
            if data["type"] == "EEG_Metrics":
                stream_name = data["name"]

                # Use the helper method to handle any type of value
                for key in ["bands", "metrics"]:
                    if key in data and data[key]:
                        base_address = f"/{stream_name}/{key}"
                        self._send_value(base_address, data[key], port)

                print(f"Sent metrics data for {stream_name} to all IPs on port {port}")
            else:
                # Generic custom message handler
                for key, value in data.items():
                    if isinstance(value, dict):
                        # Handle nested dictionary
                        for subkey, subval in value.items():
                            address = f"/{data['name']}/{key}/{subkey}"
                            self._send_value(address, subval, port)
                    else:
                        # Handle simple values
                        address = f"/{data['name']}/{key}"
                        self._send_value(address, value, port)

            # Update stats
            self.messages_sent += 1
            self.last_sent_time = time.time()
            return

        # Standard LSL data format (stream_id -> stream_data mapping)
        for stream_id, stream_data in data.items():
            try:
                stream_type = stream_data["type"]
                stream_name = stream_data["name"]
                samples = stream_data["samples"]
                timestamps = stream_data["timestamps"]

                # Get the appropriate port for this stream
                port = self._get_port_for_stream(stream_type)
                self._ensure_client_exists(port)

                # Choose the send method based on stream type
                if stream_type in ["EEG", "EXG"]:
                    # Debug print
                    print(f"Sending EEG data for {stream_name} to port {port}")
                    self._send_eeg_data(port, stream_name, samples, timestamps)
                elif stream_type in ["AUX", "Accelerometer"]:
                    # Debug print
                    print(f"Sending AUX data for {stream_name} to port {port}")
                    self._send_aux_data(port, stream_name, samples, timestamps)
                elif stream_type in ["Marker", "Markers"]:
                    # Debug print
                    print(f"Sending Marker data for {stream_name} to port {port}")
                    self._send_marker_data(port, stream_name, samples, timestamps)
                else:
                    # Generic handler for other stream types
                    # Debug print
                    print(f"Sending generic data for {stream_name} to port {port}")
                    self._send_generic_data(
                        port, stream_name, stream_type, samples, timestamps
                    )

            except Exception as e:
                print(f"Error processing stream {stream_id}: {e}")

        # Update stats
        self.messages_sent += 1
        self.last_sent_time = time.time()

    def _send_eeg_data(self, port, stream_name, samples, timestamps):
        """Send EEG data in a format suitable for Max/MSP."""
        client = self.clients[port]

        # Create base address pattern for this stream
        base_address = f"/{stream_name.replace(' ', '_')}/eeg"

        # For each sample
        for i, (sample, timestamp) in enumerate(zip(samples, timestamps)):
            # Send all channels in a single message for efficiency
            address = f"{base_address}/channels"
            client.send_message(address, sample)

            # Send timestamp separately
            client.send_message(f"{base_address}/timestamp", timestamp)

            # Debug print
            print(f"Sent EEG data: {sample} to {address} at {timestamp}")

            # Optional: Send individual channels (useful for some Max/MSP patches)
            for ch_idx, ch_value in enumerate(sample):
                client.send_message(f"{base_address}/ch{ch_idx+1}", ch_value)

    def _send_aux_data(self, port, stream_name, samples, timestamps):
        """Send auxiliary data (like accelerometer)."""
        client = self.clients[port]

        # Create base address pattern for this stream
        base_address = f"/{stream_name.replace(' ', '_')}/aux"

        # For each sample
        for i, (sample, timestamp) in enumerate(zip(samples, timestamps)):
            # Send all channels together
            address = f"{base_address}/data"
            client.send_message(address, sample)

            # Send timestamp separately
            client.send_message(f"{base_address}/timestamp", timestamp)

    def _send_marker_data(self, port, stream_name, samples, timestamps):
        """Send marker data."""
        client = self.clients[port]

        # Create base address pattern for this stream
        base_address = f"/{stream_name.replace(' ', '_')}/marker"

        # For each marker
        for i, (sample, timestamp) in enumerate(zip(samples, timestamps)):
            # For markers, the sample might be a single value or a list with one value
            marker_value = sample[0] if isinstance(sample, list) else sample

            # Send marker with its timestamp
            client.send_message(f"{base_address}/value", marker_value)
            client.send_message(f"{base_address}/timestamp", timestamp)

    def _send_generic_data(self, port, stream_name, stream_type, samples, timestamps):
        """Handle any other kind of data."""
        client = self.clients[port]

        # Create base address pattern for this stream
        base_address = f"/{stream_name.replace(' ', '_')}/{stream_type.lower()}"

        # For each sample
        for i, (sample, timestamp) in enumerate(zip(samples, timestamps)):
            # Try to send as is - if it fails, convert to a string representation
            try:
                client.send_message(f"{base_address}/data", sample)
            except (TypeError, ValueError):
                # If sending as-is fails, try to convert to string
                sample_str = json.dumps(sample)
                client.send_message(f"{base_address}/data_string", sample_str)

            # Send timestamp separately
            client.send_message(f"{base_address}/timestamp", timestamp)

    def stop(self):
        """Stop the OSC sender."""
        self.clients = {}
        print(f"OSC sender stopped. Sent {self.messages_sent} messages.")
