from pythonosc import udp_client
from pythonosc.osc_message_builder import OscMessageBuilder
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer
import numpy as np
import json
import time
import threading
from rich import print as rprint


class OSCSender:
    """
    Sends data to OSC clients (like Max/MSP or Ableton).
    Formats LSL data appropriately for OSC protocol.
    """

    def __init__(self, ip="127.0.0.1", ports=None, discovery_port=42069):
        """
        Initialize OSC sender.

        Args:
            ip: IP address(es) to send OSC messages to (default: localhost)
               Can be a single IP or a list of IPs for multiple destinations
            ports: Dictionary mapping stream types to ports or single port for all streams
                   Example: {'EEG': 9000, 'Markers': 9001} or just 9000
            discovery_port: Port to listen for discovery messages (default: 42069)
        """
        # Handle ip parameter as either a single IP or a list of IPs
        if isinstance(ip, list):
            self.ips = ip.copy()  # Make a copy to avoid modifying the original
        else:
            self.ips = [ip]  # Convert single IP to list

        # Track discovered IPs separately
        self.discovered_audio_ips = []
        self.discovered_visual_ips = []

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

        # Set up OSC discovery server
        self.discovery_port = discovery_port
        self.setup_discovery_server()

    def setup_discovery_server(self):
        """Set up an OSC server to listen for discovery messages."""
        # Configure OSC dispatcher for discovery messages
        dispatcher = Dispatcher()
        dispatcher.map("/audioComputer/discover", self._handle_audio_discovery)
        dispatcher.map("/visualComputer/discover", self._handle_visual_discovery)

        try:
            # Try to start the OSC server
            self.server = BlockingOSCUDPServer(
                ("0.0.0.0", self.discovery_port), dispatcher
            )

            # Start the server in a separate thread so it doesn't block
            self.server_thread = threading.Thread(target=self._run_server, daemon=True)
            self.server_thread.start()
            rprint(
                f"[green]OSC discovery server started on port {self.discovery_port}[/green]"
            )
        except Exception as e:
            rprint(f"[red]Failed to start OSC discovery server: {e}[/red]")
            self.server = None

    def _run_server(self):
        """Run the OSC server in a separate thread."""
        try:
            # Process incoming messages with a timeout to allow thread termination
            while True:
                self.server.handle_request()
        except Exception as e:
            rprint(f"[red]OSC server error: {e}[/red]")

    def _handle_audio_discovery(self, address, *args):
        """Handle discovery message from audio computer."""
        client_address = self.server.client_address[0]
        rprint(f"[cyan]Received audio discovery from {client_address}[/cyan]")

        # Add to discovered audio IPs if not already present
        if client_address not in self.discovered_audio_ips:
            self.discovered_audio_ips.append(client_address)

            # Create clients for this new IP
            self._create_clients_for_ip(client_address)

            # Send acknowledgment
            self._send_acknowledgment(client_address, "audio")

    def _handle_visual_discovery(self, address, *args):
        """Handle discovery message from visual computer."""
        client_address = self.server.client_address[0]
        rprint(f"[magenta]Received visual discovery from {client_address}[/magenta]")

        # Add to discovered visual IPs if not already present
        if client_address not in self.discovered_visual_ips:
            self.discovered_visual_ips.append(client_address)

            # Create clients for this new IP
            self._create_clients_for_ip(client_address)

            # Send acknowledgment
            self._send_acknowledgment(client_address, "visual")

    def _send_acknowledgment(self, ip, source_type):
        """Send acknowledgment back to the discovered computer."""
        port = self._get_port_for_stream("EEG")  # Use the EEG port for acknowledgment
        client_key = f"{ip}:{port}"

        # Create client if it doesn't exist
        if client_key not in self.clients:
            self.clients[client_key] = udp_client.SimpleUDPClient(ip, port)

        # Send acknowledgment
        self.clients[client_key].send_message("/bridge/acknowledgment", source_type)
        rprint(f"[green]Sent acknowledgment to {source_type} computer at {ip}[/green]")

    def _create_clients_for_ip(self, ip):
        """Create clients for a newly discovered IP address."""
        if isinstance(self.ports, dict):
            for stream_type, port in self.ports.items():
                client_key = f"{ip}:{port}"
                if client_key not in self.clients:
                    self.clients[client_key] = udp_client.SimpleUDPClient(ip, port)
                    rprint(
                        f"[green]Created new client for {ip}:{port} ({stream_type})[/green]"
                    )
        else:
            client_key = f"{ip}:{self.default_port}"
            if client_key not in self.clients:
                self.clients[client_key] = udp_client.SimpleUDPClient(
                    ip, self.default_port
                )
                rprint(
                    f"[green]Created new client for {ip}:{self.default_port}[/green]"
                )

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

    def start(self):
        """Initialize the OSC sender."""
        print(f"OSC sender initialized. Ready to send data to {', '.join(self.ips)}")
        for port in (
            set([self._get_port_for_stream(k) for k in self.ports.keys()])
            if isinstance(self.ports, dict)
            else [self.default_port]
        ):
            rprint(f"[green3]- OSC port {port} configured[/green3]")

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

    # Update the send_message method to format OSC messages correctly for Max
    def send_message(self, data):
        """
        Send data via OSC to all configured IP addresses.

        Args:
            data: Dictionary containing data from LSLReceiver.get_data()
        """
        # First get all effective IPs (configured + discovered)
        effective_ips = self._get_effective_ips()

        if not data:
            print("No data to send.")
            return

        # Check if this is a standard LSL data format or a custom message
        if "type" in data and isinstance(data["type"], str):
            # This appears to be a direct/custom message
            port = self._get_port_for_stream(data["type"])
            self._ensure_client_exists(port, effective_ips)

            # Handle EEG_Metrics formatted message for Max compatibility
            if data["type"] == "EEG_Metrics":
                stream_name = data["name"]
                if not stream_name.endswith("_EXG"):
                    stream_name = (
                        f"{stream_name}_EXG"  # Ensure name matches what Max expects
                    )

                # Format band powers in a Max-friendly way
                if "bands" in data and data["bands"]:
                    for band_name, band_value in data["bands"].items():
                        # Format: /stream_name/bands/band_name value
                        address = f"/{stream_name}/bands/{band_name}"
                        for ip in effective_ips:
                            client_key = f"{ip}:{port}"
                            if client_key in self.clients:
                                try:
                                    self.clients[client_key].send_message(
                                        address, float(band_value)
                                    )
                                except (TypeError, ValueError) as e:
                                    rprint(
                                        f"[red]Error sending band {band_name}: {e} - Value type: {type(band_value)}[/red]"
                                    )
                                    # If it's a complex type, try sending a representative value
                                    if isinstance(band_value, dict) and band_value:
                                        # Send first value from the dictionary
                                        first_value = next(iter(band_value.values()))
                                        self.clients[client_key].send_message(
                                            address, float(first_value)
                                        )
                                    elif (
                                        isinstance(band_value, (list, tuple))
                                        and band_value
                                    ):
                                        # Send first value from list/tuple
                                        self.clients[client_key].send_message(
                                            address, float(band_value[0])
                                        )
                                    else:
                                        # Send 0 as fallback
                                        self.clients[client_key].send_message(
                                            address, 0.0
                                        )

                # Format metrics in a Max-friendly way
                if "metrics" in data and data["metrics"]:
                    for metric_name, metric_value in data["metrics"].items():
                        # Format: /stream_name/metrics/metric_name value
                        address = f"/{stream_name}/metrics/{metric_name}"
                        for ip in effective_ips:
                            client_key = f"{ip}:{port}"
                            if client_key in self.clients:
                                try:
                                    # Handle complex types
                                    if isinstance(metric_value, dict):
                                        # For dictionaries, send first value or 0
                                        if metric_value:
                                            first_value = next(
                                                iter(metric_value.values())
                                            )
                                            self.clients[client_key].send_message(
                                                address, float(first_value)
                                            )
                                        else:
                                            self.clients[client_key].send_message(
                                                address, 0.0
                                            )
                                    elif (
                                        isinstance(metric_value, (list, tuple))
                                        and metric_value
                                    ):
                                        # For lists/tuples, send first value
                                        self.clients[client_key].send_message(
                                            address, float(metric_value[0])
                                        )
                                    else:
                                        # Try direct conversion
                                        self.clients[client_key].send_message(
                                            address, float(metric_value)
                                        )
                                except (TypeError, ValueError) as e:
                                    rprint(
                                        f"[red]Error sending metric {metric_name}: {e}[/red]"
                                    )
                                    # Send 0 as fallback
                                    self.clients[client_key].send_message(address, 0.0)
            elif data["type"] == "EEG_Raw":
                stream_name = data["name"]

                # Format: /stream_name/eeg/channels [ch1, ch2, ch3, ...]
                address = f"/{stream_name}/eeg/channels"
                for ip in effective_ips:
                    client_key = f"{ip}:{port}"
                    if client_key in self.clients:
                        try:
                            # Send all channels in a single message
                            self.clients[client_key].send_message(
                                address, data["channels"]
                            )
                            rprint(
                                f"[sky_blue2]Sent raw EEG data with {len(data['channels'])} channels[/sky_blue2]"
                            )
                        except Exception as e:
                            rprint(f"[red]Error sending raw EEG data: {e}[/red]")
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
                self._ensure_client_exists(port, effective_ips)

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

    def _get_effective_ips(self):
        """Get all effective IPs (configured + discovered)."""
        # Combine all sources of IPs, removing duplicates
        all_ips = set(self.ips)
        all_ips.update(self.discovered_audio_ips)
        all_ips.update(self.discovered_visual_ips)
        return list(all_ips)

    def _ensure_client_exists(self, port, ips=None):
        """Make sure a client exists for the given port and IPs."""
        if ips is None:
            ips = self._get_effective_ips()

        for ip in ips:
            client_key = f"{ip}:{port}"
            if client_key not in self.clients:
                self.clients[client_key] = udp_client.SimpleUDPClient(ip, port)

    def stop(self):
        """Stop the OSC sender and discovery server."""
        if hasattr(self, "server") and self.server:
            self.server.server_close()

        self.clients = {}
        print(f"OSC sender stopped. Sent {self.messages_sent} messages.")
