#include <Wire.h>

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  while (!Serial);
  
  Serial.println("--- SYSTEM START ---");
  
  Wire.begin(0x3C);
  Wire.onReceive(receiveEvent);
  
  Serial.println("I2C Target Ready at 0x3C");
}

void loop() {
  // Heartbeat blink
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
  Serial.println("Waiting for I2C...");
}

void receiveEvent(int howMany) {
  Serial.print("ACK Received! Bytes: ");
  Serial.println(howMany);
  while (Wire.available()) {
    byte c = Wire.read();
    Serial.println(c, HEX);
  }
}
