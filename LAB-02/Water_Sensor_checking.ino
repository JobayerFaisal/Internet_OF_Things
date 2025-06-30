#define SIGNAL_PIN A5
int value = 0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
value = analogRead(SIGNAL_PIN);

  Serial.print("sensor value:");
  Serial.println("sensor");

  delay(1000);
}
