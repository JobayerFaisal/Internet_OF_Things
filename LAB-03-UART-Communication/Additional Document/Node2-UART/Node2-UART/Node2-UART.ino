#include <SoftwareSerial.h>

// Define SoftwareSerial pins
SoftwareSerial mySerial(D6, D5); // RX = D6, TX = D5

void setup() {
  Serial.begin(115200); // Debugging
  mySerial.begin(9600); // UART
  Serial.println("NodeMCU 2: UART Slave");
}

void loop() {
  if (mySerial.available()) {
    String receivedData = mySerial.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(receivedData);
    
    mySerial.println("Hi from NodeMCU 2");
    Serial.println("Sent: Hi from NodeMCU 2");
  }
  
  delay(100); // Small delay
}