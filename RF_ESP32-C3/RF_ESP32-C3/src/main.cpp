#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(2, 3); // CE, CSN pins

void setup() {
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println("Radio init failed");
    while (1);
  }

  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76); // Try different channels (0-125)
  radio.enableDynamicPayloads(); // <-- Add this line
  radio.openReadingPipe(0, 0xF0F0F0F0E1LL); // Use a non-zero 5-byte address
  radio.startListening();

  Serial.println("Listening for signals...");
}

void loop() {
  if (radio.available()) {
    uint8_t len = radio.getDynamicPayloadSize();
    if (len > 0 && len <= 32) {
      uint8_t data[32];
      radio.read(&data, len);

      Serial.print("Received data (len ");
      Serial.print(len);
      Serial.print("): ");
      for (int i = 0; i < len; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}