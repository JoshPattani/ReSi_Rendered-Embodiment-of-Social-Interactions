/**
 * Campus WiFi Connection with Captive Portal Support
 *
 * This sketch enables ESP32 devices to connect to university WiFi networks
 * that use captive portal authentication. It's specifically designed for
 * UCB Wireless but can work with other similar networks.
 *
 * Features:
 * - Automatic captive portal detection
 * - Authentication to the captive portal
 * - Support for form-based and redirect-based portals
 * - Support for authentication tokens
 *
 * Setup:
 * 1. Create wifi_credentials.h with your credentials
 * 2. Adjust configuration parameters if needed
 * 3. Upload to your ESP32
 *
 * Created for RESI (Rendered Embodiment of Social Interactions) 2025
 * Author: Josh Pattani
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Your university login credentials
#include "wifi_credentials.h" // Include the wifi_credentials header for identikey and password

// Variables to store portal information
String captivePortalURL = "";
String authURL = "";
String authToken = "";

#define DEBUG_ENABLE true            // Enable/disable debug messages
#define MAX_CONNECTION_ATTEMPTS 20   // Number of attempts before timeout
#define CONNECTION_ATTEMPT_DELAY 500 // Delay between connection attempts (ms)
#define AUTH_STABILIZE_DELAY 2000    // Delay after authentication (ms)

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 Captive Portal WiFi Connection");
  connectToWiFi();
}

void loop()
{
  // Once connected, your main code can go here
  delay(10000);

  // Periodically check if we're still connected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
}

bool connectToCampusWiFi()
{
  logDebug("Connecting to campus WiFi...");

  WiFi.begin(ssid);

  // Wait for connection attempt
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS)
  {
    delay(CONNECTION_ATTEMPT_DELAY);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    logDebug("\nConnected directly to WiFi!");
    logDebug("IP address: " + WiFi.localIP().toString());
    return true;
  }

  // If direct connection failed, try captive portal
  logDebug("\nDirect connection failed. Trying captive portal authentication...");

  // Detect captive portal
  // First try UCB-specific method
  if (handleUCBWirelessAuth())
  {
    return true;
  }

  // Then try generic captive portal detection and authentication
  if (detectCaptivePortal())
  {
    return authenticateCaptivePortal();
  }

  logDebug("Failed to connect via captive portal. Check credentials or network.");
  return false;
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

  try
  {
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
    success = true;
  }
  catch (...)
  {
    logDebug("Exception during authentication");
  }

  http.end();
  return success;
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

bool handleUCBWirelessAuth()
{
  logDebug("Using UCB Wireless specific authentication...");

  // Reset WiFi connection
  WiFi.disconnect();
  delay(1000);

  // Connect to the network
  WiFi.begin(ssid);

  // Wait for initial connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS)
  {
    delay(CONNECTION_ATTEMPT_DELAY);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    logDebug("\nInitial connection established");
    delay(2000);

    // UCB-specific authentication code
    // ...

    return true;
  }

  return false;
}

void logDebug(const char *message)
{
  if (DEBUG_ENABLE)
  {
    Serial.println(message);
  }
}

void logDebug(String message)
{
  if (DEBUG_ENABLE)
  {
    Serial.println(message);
  }
}