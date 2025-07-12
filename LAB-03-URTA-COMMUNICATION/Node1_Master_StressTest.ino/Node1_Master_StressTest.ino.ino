// MASTER REVEIVER 
 
 #include <SoftwareSerial.h>

// Define SoftwareSerial pins
SoftwareSerial mySerial(D6, D5); // RX = D6, TX = D5

// Test parameters
const int baudRates[] = {9600, 38400, 115200};
//const int baudRates[] = {230400, 250000};

const int messageSizes[] = {100, 50, 10};
//const int intervals[] = {0, 10, 100}; // Delay in ms

const int intervals[] = {100, 100, 10}; // Delay in ms
const unsigned long testDuration = 10000; // 10 seconds per test
const int timeout = 500; // 500ms timeout for response

void setup() {
  Serial.begin(460800); // Debugging
  Serial.println("NodeMCU 1: UART Master Stress Test");
}

void loop() {
  // Run tests for each baud rate
  for (int baud : baudRates) {
    mySerial.begin(baud);
    Serial.print("Testing baud rate: ");
    Serial.println(baud);
    // Send baud rate change command to slave
    mySerial.print("BAUD:");
    mySerial.println(baud);
    delay(000); // Allow NodeMCU 2 to sync baud rate

    // Run tests for each message size
    for (int size : messageSizes) {
      // Run tests for each interval
      for (int interval : intervals) {
        runTest(baud, size, interval);
        delay(2000); // Pause between tests
      }
    }
    mySerial.end();
  }
  Serial.println("All tests complete. Restarting in 10 seconds...");
  delay(10000);
}

/*void runTest(int baud, int size, int interval) {
  Serial.print("Test: Baud=");
  Serial.print(baud);
  Serial.print(", Size=");
  Serial.print(size);
  Serial.print(" bytes, Interval=");
  Serial.print(interval);
  Serial.println("ms");

  unsigned long startTime = millis();
  int messagesSent = 0;
  int messagesReceived = 0;
  int errors = 0;
  unsigned long bytesSent = 0;
  int sequence = 0;

  // Generate test message
  String baseMessage = "D";
  while (baseMessage.length() < size - 6) { // Reserve 6 bytes for sequence
    baseMessage += "X";
  }

  // Run test for 10 seconds
  while (millis() - startTime < testDuration) {
    // Send message with sequence number
    String message = String(sequence) + ":" + baseMessage;
    mySerial.println(message);
    bytesSent += message.length() + 2; // Include \r\n
    messagesSent++;
    sequence++;

    // Wait for response with timeout
    unsigned long responseStart = millis();
    String receivedData = "";
    bool received = false;

    while (millis() - responseStart < timeout) {
      if (mySerial.available()) {
        char c = mySerial.read();
        if (c == '\n') {
          received = true;
          break;
        }
        receivedData += c;
      }
    }

    // Verify response
    if (received && receivedData == message) {
      messagesReceived++;
    } else {
      errors++;
      if (!received) {
        Serial.println("Timeout error");
      } else {
        Serial.print("Mismatch error: Expected '");
        Serial.print(message);
        Serial.print("', Got '");
        Serial.print(receivedData);
        Serial.println("'");
      }
    }

    // Delay for specified interval
    if (interval > 0) {
      delay(interval);
    }
  }

  // Calculate and print results
  float durationSeconds = testDuration / 1000.0;
  float throughput = bytesSent / durationSeconds; // Bytes per second
  float messageRate = messagesSent / durationSeconds; // Messages per second
  float errorRate = (errors * 100.0) / messagesSent; // Percentage

  Serial.println("Test Results:");
  Serial.print("Messages Sent: ");
  Serial.println(messagesSent);
  Serial.print("Messages Received: ");
  Serial.println(messagesReceived);
  Serial.print("Errors: ");
  Serial.println(errors);
  Serial.print("Error Rate: ");
  Serial.print(errorRate);
  Serial.println("%");
  Serial.print("Throughput: ");
  Serial.print(throughput);
  Serial.println(" bytes/second");
  Serial.print("Message Rate: ");
  Serial.print(messageRate);
  Serial.println(" messages/second");
  Serial.println();
}*/

void runTest(int baud, int size, int interval) {
  Serial.print("Test: Baud=");
  Serial.print(baud);
  Serial.print(", Size=");
  Serial.print(size);
  Serial.print(" bytes, Interval=");
  Serial.print(interval);
  Serial.println("ms");

  unsigned long startTime = millis();
  int messagesSent = 0;
  int messagesReceived = 0;
  int errors = 0;
  unsigned long bytesSent = 0;
  int sequence = 0;

  // Generate test message
  String baseMessage = "D";
  while (baseMessage.length() < size - 6) { // Reserve 6 bytes for sequence
    baseMessage += "X";
  }

  // Run test for 10 seconds
  while (millis() - startTime < testDuration) {
    // Send message with sequence number
    String message = String(sequence) + ":" + baseMessage;
    mySerial.println(message);
    bytesSent += message.length() + 2; // Include \r\n
    messagesSent++;
    sequence++;

    // Wait for response with timeout
    unsigned long responseStart = millis();
    String receivedData = "";
    bool received = false;

    while (millis() - responseStart < timeout) {
      if (mySerial.available()) {
        char c = mySerial.read();
        if (c == '\n') {
          received = true;
          break;
        }
        receivedData += c;
      }
    }

    // Trim received data to remove \r or whitespace
    receivedData.trim();

    // Verify response
    if (received && receivedData == message) {
      messagesReceived++;
    } else {
      errors++;
      if (!received) {
        Serial.println("Timeout error");
      } else {
        Serial.print("Mismatch error: Expected '");
        Serial.print(message);
        Serial.print("', Got '");
        Serial.print(receivedData);
        Serial.println("'");
      }
    }

    // Delay for specified interval
    if (interval > 0) {
      delay(interval);
    }
  }

  // Calculate and print results
  float durationSeconds = testDuration / 1000.0;
  float throughput = bytesSent / durationSeconds; // Bytes per second
  float messageRate = messagesSent / durationSeconds; // Messages per second
  float errorRate = (errors * 100.0) / messagesSent; // Percentage

  Serial.println("Test Results:");
  Serial.print("Messages Sent: ");
  Serial.println(messagesSent);
  Serial.print("Messages Received: ");
  Serial.println(messagesReceived);
  Serial.print("Errors: ");
  Serial.println(errors);
  Serial.print("Error Rate: ");
  Serial.print(errorRate);
  Serial.println("%");
  Serial.print("Throughput: ");
  Serial.print(throughput);
  Serial.println(" bytes/second");
  Serial.print("Message Rate: ");
  Serial.print(messageRate);
  Serial.println(" messages/second");
  Serial.println();
}