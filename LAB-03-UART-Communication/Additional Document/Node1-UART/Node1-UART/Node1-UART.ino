#include <SoftwareSerial.h>

// Define SoftwareSerial pins
SoftwareSerial mySerial(D6, D5); // RX = D6, TX = D5

void setup() {
  Serial.begin(115200); // Debugging
  mySerial.begin(9600); // UART
  Serial.println("NodeMCU 1: UART Master");
}

void loop() {
  mySerial.println("Hello from NodeMCU 1");
  Serial.println("Sent: Hello from NodeMCU 1");
  
  if (mySerial.available()) {
    String receivedData = mySerial.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(receivedData);
  }
  
  delay(2000); // Wait 2 seconds
}