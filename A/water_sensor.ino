int sensorPin = A0;   // Connect AO to analog pin A0
int sensorValue = 0;  // Variable to store sensor reading

void setup() {
  Serial.begin(9600);  // Start Serial Monitor
}

void loop() {
  sensorValue = analogRead(sensorPin);  // Read analog value
  Serial.print("Water Level Value: ");
  Serial.println(sensorValue);          // Print to Serial Monitor
  delay(1000);                          // Delay 1 second
}
