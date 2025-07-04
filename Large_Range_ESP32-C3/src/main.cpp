#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// WiFi credentials
const char* ssid = "Castillo Fuerte";
const char* password = "salmos18:2";

// Server endpoint - FIXED: Added http:// and /solar-log path
const char* serverURL = "http://aschie.mooo.com:4545/solar-log";

// Time configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;  // Adjust for your timezone
const int daylightOffset_sec = 3600;  // Adjust for daylight saving

// Timing
unsigned long lastLogTime = 0;
// Choose one of these intervals:
const unsigned long logInterval = 2 * 60 * 1000;  // 1 hour in milliseconds
// const unsigned long logInterval = 2 * 60 * 60 * 1000;  // 2 hours in milliseconds
// const unsigned long logInterval = 4 * 60 * 60 * 1000;  // 4 hours in milliseconds
// const unsigned long logInterval = 6 * 60 * 60 * 1000;  // 6 hours in milliseconds
// const unsigned long logInterval = 12 * 60 * 60 * 1000; // 12 hours in milliseconds

void sendLogToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Add more debug info
    Serial.println("Attempting to connect to: " + String(serverURL));
    Serial.println("WiFi RSSI: " + String(WiFi.RSSI()));
    
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000); // 10 second timeout
    
    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Create JSON payload - FIXED: Using JsonDocument instead of StaticJsonDocument
    JsonDocument doc;
    doc["device_id"] = "esp32_solar_monitor";
    doc["timestamp"] = now;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.println("Sending JSON: " + jsonString);
    
    // Send POST request
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Log sent successfully! Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error sending log. Response code: " + String(httpResponseCode));
      
      // More detailed error info
      if (httpResponseCode == -1) {
        Serial.println("Connection failed - check server URL and network");
      } else if (httpResponseCode == -11) {
        Serial.println("Timeout - server may be slow or unreachable");
      }
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected, cannot send log");
  }
}

// Optional: Function to enter deep sleep to save power
void enterDeepSleep(int minutes) {
  Serial.println("Entering deep sleep for " + String(minutes) + " minutes");
  esp_sleep_enable_timer_wakeup(minutes * 60 * 1000000ULL); // Convert to microseconds
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  
  // Conectar WiFi
  Serial.println("\nConectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado al WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait for time to be set
  Serial.print("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  
  // Send initial log
  sendLogToServer();
  lastLogTime = millis();
}

void loop() {
  // Check if it's time to log
  if (millis() - lastLogTime >= logInterval) {
    sendLogToServer();
    lastLogTime = millis();
  }
  
  // Check WiFi connection with fast reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting fast reconnect...");
    // Try fast reconnect first
    WiFi.reconnect();

    }

  // Sleep for a bit to save power
  delay(1000); // Check every 1 seconds
}