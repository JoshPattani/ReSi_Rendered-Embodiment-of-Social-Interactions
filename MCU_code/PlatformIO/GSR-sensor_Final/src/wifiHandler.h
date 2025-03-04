#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "arduino_secrets.h"

// Add these external declarations to access needed variables
extern const IPAddress outIp;
extern const unsigned int outPort;

// WiFi credentials
const char *ssid = SECRET_UCB_SSID; // Your WiFi SSID

// Your university login credentials
const char *identikey = SECRET_IDENTIKEY; // Your university identikey
const char *password = SECRET_PASS;       // Your university password

// Variables to store portal information
String captivePortalURL = "";
String authURL = "";
String authToken = "";

String extractLoginURL(String html)
{
    // This is a simplified extraction that looks for common patterns
    // You may need to customize this for your specific captive portal

    // Look for forms with post action
    int formIndex = html.indexOf("<form");
    if (formIndex > 0)
    {
        int actionIndex = html.indexOf("action=", formIndex);
        if (actionIndex > 0)
        {
            int quoteChar = html.charAt(actionIndex + 7); // Either ' or "
            int urlStart = actionIndex + 8;
            int urlEnd = html.indexOf(quoteChar, urlStart);

            String formAction = html.substring(urlStart, urlEnd);

            // If it's a relative URL, we need to prepend the base URL
            if (formAction.startsWith("/"))
            {
                // Extract base URL from the current page
                String baseURL = "";
                int httpIndex = html.indexOf("http");
                if (httpIndex >= 0)
                {
                    int slashAfterDomain = html.indexOf("/", httpIndex + 8);
                    if (slashAfterDomain > 0)
                    {
                        baseURL = html.substring(httpIndex, slashAfterDomain);
                    }
                }

                return baseURL + formAction;
            }

            return formAction;
        }
    }

    // If we couldn't find a form, look for redirect URLs
    int redirectIndex = html.indexOf("window.location");
    if (redirectIndex > 0)
    {
        int urlStart = html.indexOf("'", redirectIndex);
        if (urlStart < 0)
        {
            urlStart = html.indexOf("\"", redirectIndex);
        }

        if (urlStart > 0)
        {
            int quoteChar = html.charAt(urlStart);
            urlStart++;
            int urlEnd = html.indexOf(quoteChar, urlStart);

            return html.substring(urlStart, urlEnd);
        }
    }

    // Extract auth token if present
    int tokenIndex = html.indexOf("name=\"auth_token\"");
    if (tokenIndex > 0)
    {
        int valueIndex = html.indexOf("value=", tokenIndex);
        if (valueIndex > 0)
        {
            int quoteChar = html.charAt(valueIndex + 6); // Either ' or "
            int tokenStart = valueIndex + 7;
            int tokenEnd = html.indexOf(quoteChar, tokenStart);

            authToken = html.substring(tokenStart, tokenEnd);
            Serial.print("Found auth token: ");
            Serial.println(authToken);
        }
    }

    return ""; // Couldn't find a login URL
}

bool detectCaptivePortal()
{
    HTTPClient http;

    // Try to connect to a known website that should redirect to the captive portal
    // Apple's captive portal detection URL
    http.begin("http://captive.apple.com/hotspot-detect.html");

    int httpCode = http.GET();
    Serial.print("HTTP GET status code: ");
    Serial.println(httpCode);

    if (httpCode > 0)
    {
        String payload = http.getString();
        http.end();

        // Check if we got redirected (captive portal will usually return 302)
        if (httpCode == 302 || payload.indexOf("login") > 0 || payload.indexOf("auth") > 0)
        {
            // Try to extract the login URL from the response
            captivePortalURL = extractLoginURL(payload);

            if (captivePortalURL.length() > 0)
            {
                Serial.print("Detected captive portal URL: ");
                Serial.println(captivePortalURL);
                return true;
            }
        }
    }

    http.end();

    // Alternative method: Try to detect by checking for DNS hijacking
    http.begin("http://example.com");
    httpCode = http.GET();

    if (httpCode > 0)
    {
        String payload = http.getString();
        http.end();

        // If we don't get the expected response from example.com, we're likely in a captive portal
        if (payload.indexOf("login") > 0 || payload.indexOf("auth") > 0 || payload.indexOf("captive") > 0)
        {
            captivePortalURL = extractLoginURL(payload);

            if (captivePortalURL.length() > 0)
            {
                Serial.print("Detected captive portal URL (alt method): ");
                Serial.println(captivePortalURL);
                return true;
            }
        }
    }

    http.end();
    return false;
}

// URL encoding function
String urlEncode(String str)
{
    String encodedString = "";
    char c;
    char code0;
    char code1;

    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);

        if (c == ' ')
        {
            encodedString += '+';
        }
        else if (isAlphaNumeric(c))
        {
            encodedString += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
                code1 = (c & 0xf) - 10 + 'A';
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9)
                code0 = c - 10 + 'A';
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }

    return encodedString;
}

bool authenticateCaptivePortal()
{
    if (captivePortalURL.length() == 0)
    {
        Serial.println("No captive portal URL detected!");
        return false;
    }

    HTTPClient http;
    http.begin(captivePortalURL);

    // First, GET the login page to extract any necessary tokens or cookies
    int httpCode = http.GET();
    Serial.print("Login page GET status: ");
    Serial.println(httpCode);

    if (httpCode > 0)
    {
        String loginPage = http.getString();

        // Extract auth token if not already done
        if (authToken.length() == 0)
        {
            int tokenIndex = loginPage.indexOf("name=\"auth_token\"");
            if (tokenIndex > 0)
            {
                int valueIndex = loginPage.indexOf("value=", tokenIndex);
                if (valueIndex > 0)
                {
                    int quoteChar = loginPage.charAt(valueIndex + 6); // Either ' or "
                    int tokenStart = valueIndex + 7;
                    int tokenEnd = loginPage.indexOf(quoteChar, tokenStart);

                    authToken = loginPage.substring(tokenStart, tokenEnd);
                    Serial.print("Found auth token: ");
                    Serial.println(authToken);
                }
            }
        }

        // Extract the form action URL if different from captive portal URL
        int formIndex = loginPage.indexOf("<form");
        if (formIndex > 0)
        {
            int actionIndex = loginPage.indexOf("action=", formIndex);
            if (actionIndex > 0)
            {
                int quoteChar = loginPage.charAt(actionIndex + 7); // Either ' or "
                int urlStart = actionIndex + 8;
                int urlEnd = loginPage.indexOf(quoteChar, urlStart);

                String formAction = loginPage.substring(urlStart, urlEnd);

                // If it's a relative URL, we need to prepend the base URL
                if (formAction.startsWith("/"))
                {
                    // Extract domain from the captive portal URL
                    int protocolEnd = captivePortalURL.indexOf("://");
                    int pathStart = captivePortalURL.indexOf("/", protocolEnd + 3);
                    String domain = "";

                    if (pathStart > 0)
                    {
                        domain = captivePortalURL.substring(0, pathStart);
                    }
                    else
                    {
                        domain = captivePortalURL;
                    }

                    authURL = domain + formAction;
                }
                else
                {
                    authURL = formAction;
                }
            }
        }

        if (authURL.length() == 0)
        {
            // If we couldn't extract a specific form action, use the captive portal URL
            authURL = captivePortalURL;
        }

        Serial.print("Auth URL: ");
        Serial.println(authURL);

        // Now submit the login form
        http.end();
        http.begin(authURL);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Prepare POST data - adjust field names as needed for your captive portal
        String postData = "username=" + urlEncode(identikey) +
                          "&password=" + urlEncode(password);

        // Add auth token if found
        if (authToken.length() > 0)
        {
            postData += "&auth_token=" + urlEncode(authToken);
        }

        Serial.println("Sending authentication request...");
        httpCode = http.POST(postData);

        Serial.print("Auth POST status: ");
        Serial.println(httpCode);

        if (httpCode > 0)
        {
            String response = http.getString();

            // Check if authentication was successful
            if (httpCode == 200 || httpCode == 302)
            {
                // Wait a moment for the connection to stabilize
                delay(2000);

                // Check if we're now connected
                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.println("Successfully authenticated to WiFi!");
                    Serial.print("IP address: ");
                    Serial.println(WiFi.localIP());
                    http.end();
                    return true;
                }
            }

            // If we're here, authentication might have failed
            Serial.println("Authentication may have failed. Response excerpt:");
            Serial.println(response.substring(0, 200) + "..."); // Print first 200 chars
        }
    }

    http.end();
    return false;
}

// Enumeration for different network types
enum NetworkType
{
    NETWORK_HOME,
    NETWORK_STUDIO,
    NETWORK_CAMPUS,
    NETWORK_UNKNOWN
};

// Function to scan and identify available networks
NetworkType detectNetworkType()
{
    Serial.println("Scanning for known networks...");
    int numNetworks = WiFi.scanNetworks();

    for (int i = 0; i < numNetworks; i++)
    {
        String ssid = WiFi.SSID(i);
        Serial.print("Found network: ");
        Serial.println(ssid);

        if (ssid == SECRET_HOME_SSID)
        {
            return NETWORK_HOME;
        }
        else if (ssid == SECRET_STUDIO_SSID)
        {
            return NETWORK_STUDIO;
        }
        else if (ssid == SECRET_UCB_SSID)
        {
            return NETWORK_CAMPUS;
        }
    }

    return NETWORK_UNKNOWN;
}

// Smart WiFi connection function that handles different network types
bool smartConnectWiFi()
{
    NetworkType networkType = detectNetworkType();
    bool connected = false;
    int attempts = 0;

    switch (networkType)
    {
    case NETWORK_HOME:
        Serial.println("Detected home network");
        // Let Arduino IoT Cloud handle WiFi connection later
        return true;

    case NETWORK_STUDIO:
        Serial.println("Detected studio network");
        // Let Arduino IoT Cloud handle WiFi connection later
        return true;

    case NETWORK_CAMPUS:
        Serial.println("Detected campus network with captive portal");
        // Use our captive portal handler
        WiFi.begin(SECRET_UCB_SSID);

        // Wait for initial connection attempt
        attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20)
        {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        // Try captive portal authentication
        if (detectCaptivePortal())
        {
            connected = authenticateCaptivePortal();
        }

        if (connected)
        {
            Serial.println("Successfully connected to campus WiFi");
            return true;
        }
        else
        {
            Serial.println("Failed to connect to campus WiFi");
            return false;
        }

    case NETWORK_UNKNOWN:
    default:
        Serial.println("No known networks found");
        return false;
    }

    return false;
}

// Add this global variable to track the selected network
NetworkType currentNetworkType = NETWORK_UNKNOWN;

// Update the existing connectToWiFi function to use our smart connection
void connectToWiFi()
{
    Serial.println("Starting smart WiFi connection...");
    currentNetworkType = detectNetworkType();

    // For campus network, handle captive portal first
    if (currentNetworkType == NETWORK_CAMPUS)
    {
        if (smartConnectWiFi())
        {
            Serial.println("Successfully connected to campus network");
        }
        else
        {
            Serial.println("Failed to connect to campus network");
        }
    }
    // For home/studio networks, we'll let Arduino IoT Cloud handle it later
    // Just inform the user which network we detected
    else if (currentNetworkType == NETWORK_HOME)
    {
        Serial.println("Home network detected, will connect through Arduino IoT Cloud");
    }
    else if (currentNetworkType == NETWORK_STUDIO)
    {
        Serial.println("Studio network detected, will connect through Arduino IoT Cloud");
    }
    else
    {
        Serial.println("No known networks found, will try default connection method");
    }
}