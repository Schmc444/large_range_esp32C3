#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// WiFi credentials
const char* ssid = "Castillo Fuerte";
const char* password = "salmos18:2";

// Server endpoint
const char* serverURL = "aschie.mooo.com:4545";

// Time configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;  // Adjust for your timezone
const int daylightOffset_sec = 3600;  // Adjust for daylight saving

// Timing
unsigned long lastLogTime = 0;
const unsigned long logInterval = 30 * 60 * 1000; // 30 minutes in milliseconds

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
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
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.reconnect();
    delay(5000);
  }
  
  // Sleep for a bit to save power
  delay(10000); // Check every 10 seconds
}

void sendLogToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    
    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Create JSON payload
    StaticJsonDocument<200> doc;
    doc["device_id"] = "esp32_solar_monitor";
    doc["timestamp"] = now;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send POST request
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Log sent successfully");
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error sending log: " + String(httpResponseCode));
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