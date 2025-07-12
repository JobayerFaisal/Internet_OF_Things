// SLAVE SENDER

#include <SoftwareSerial.h>

// Define SoftwareSerial pins
SoftwareSerial mySerial(D6, D5); // RX = D6, TX = D5

void setup() {
  Serial.begin(460800); // Debugging
  mySerial.begin(9600); // Start with 9600
  Serial.println("NodeMCU 2: UART Slave Initialized");
}

void loop() {
  // Check for baud rate sync command
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("BAUD:")) {
      int baud = command.substring(5).toInt();
      mySerial.end();
      mySerial.begin(baud);
      Serial.print("NodeMCU 2: Switched to baud ");
      Serial.println(baud);
    }
  }

  // Process incoming UART data
  if (mySerial.available()) {
    String receivedData = mySerial.readStringUntil('\n');
    receivedData.trim();
    Serial.print("NodeMCU 2: Received '");
    Serial.print(receivedData);
    Serial.println("'");

    // Echo the message back
    mySerial.println(receivedData);
    Serial.println("NodeMCU 2: Echoed message");
  }
}