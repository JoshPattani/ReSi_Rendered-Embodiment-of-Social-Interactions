import time
import requests
from pythonosc import udp_client

# Configuration
API_URL = "https://api.arduino.cc/iot/v2/things/YOUR_THING_ID/properties"  # Adjust URL as needed
HEADERS = {"Authorization": "Bearer YOUR_API_TOKEN", "Content-Type": "application/json"}
POLL_INTERVAL = 1  # seconds

# OSC client configuration (set these to your MAX patch's IP and port)
OSC_IP = "127.0.0.1"
OSC_PORT = 8000
client = udp_client.SimpleUDPClient(OSC_IP, OSC_PORT)


def fetch_sensor_data():
    try:
        response = requests.get(API_URL, headers=HEADERS)
        response.raise_for_status()
        data = response.json()
        # Adjust parsing according to the actual structure returned by the Arduino IoT Cloud API
        sensor_value = data.get("sensorValue", None)
        return sensor_value
    except Exception as e:
        print(f"Error fetching data: {e}")
        return None


if __name__ == "__main__":
    while True:
        sensor_value = fetch_sensor_data()
        if sensor_value is not None:
            print(f"Sending sensor value: {sensor_value}")
            # Send OSC message with address "/sensor" and the sensor_value as an argument
            client.send_message("/sensor", sensor_value)
        time.sleep(POLL_INTERVAL)
