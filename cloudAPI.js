const fetch = require('node-fetch');

// Replace these with your credentials
const CLIENT_ID = 'ri9Q46pxaEtjE0YVmmgb5edS4Rzqkp9n';
const CLIENT_SECRET = 'zssP9jpg97sRNekS0Fa27vdRw9XNIyhat86AFyKLUMPGE7Q4UaxpsIWWgRP8ly5E';
const THING_ID = 'e7aa257d-ec8c-4987-9aa9-a436d2d283ef';
const VARIABLE_ID = '63e1bf56-3ae2-406e-b0f9-a693099e281c';

let accessToken = null;

// Function to fetch a new access token
async function getAccessToken() {
    try {
        const response = await fetch('https://api2.arduino.cc/iot/v1/clients/token', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: new URLSearchParams({
                grant_type: 'client_credentials',
                client_id: CLIENT_ID,
                client_secret: CLIENT_SECRET,
            }),
        });

        const data = await response.json();
        if (response.ok) {
            accessToken = data.access_token;
            post('Access token obtained successfully.\n');
        } else {
            throw new Error(data.error || 'Failed to obtain access token');
        }
    } catch (error) {
        post(`Error obtaining access token: ${error.message}\n`);
    }
}

// Function to fetch the sensor value
async function fetchSensorValue() {
    if (!accessToken) {
        await getAccessToken(); // Ensure access token is available
    }

    try {
        const url = `https://api2.arduino.cc/iot/v2/things/${THING_ID}/properties/${VARIABLE_ID}`;
        const response = await fetch(url, {
            method: 'GET',
            headers: {
                'Authorization': `Bearer ${accessToken}`,
                'Content-Type': 'application/json',
            },
        });

        const data = await response.json();
        if (response.ok) {
            post(`Sensor Value: ${data.last_value}\n`);
        } else {
            throw new Error(data.message || 'Failed to fetch sensor value');
        }
    } catch (error) {
        post(`Error fetching sensor value: ${error.message}\n`);
    }
}

// Schedule periodic API calls
setInterval(fetchSensorValue, 2000);