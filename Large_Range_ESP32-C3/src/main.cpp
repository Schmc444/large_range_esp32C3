#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>

// AP credentials
const char* ap_ssid = "ESP32-OTA";
const char* ap_password = "12345678";  // Min 8 chars

WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // Start Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());  // Usually 192.168.4.1
  
  // Configure OTA
  ArduinoOTA.setHostname("esp32-device");
  ArduinoOTA.setPassword("otapassword");  // Optional but recommended
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update starting...");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA Update finished!");
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
  });
  
  ArduinoOTA.begin();
  
  // Simple web server for status
  server.on("/", []() {
    server.send(200, "text/plain", "ESP32 OTA Ready");
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  delay(10);
}