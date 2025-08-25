#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// --- USER SETTINGS ---
const char* WIFI_SSID = "Diba";
const char* WIFI_PASSWORD = "89718971";
const char* MQTT_SERVER = "demo.thingsboard.io";  // Change to your server IP
const uint16_t MQTT_PORT = 1883;
const char* ACCESS_TOKEN = "exh1PPkTNLLEki8ARUCI"; // Use the device token

#define DHTPIN D4         // Pin for DHT22 (GPIO2)
#define DHTTYPE DHT22     // DHT22 sensor type
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;  // 5 seconds interval

// MQ3 sensor (analog pin)
#define MQ3_PIN A0       // MQ3 analog pin (A0)

// Connect to WiFi
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// Connect to MQTT server
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP8266Client", ACCESS_TOKEN, "")) {
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// Setup MQTT client
void setup() {
  Serial.begin(115200);
  delay(10);
  dht.begin();
  connectWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  // Keep WiFi and MQTT alive
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected()) connectMQTT();
  client.loop();

  // Read DHT22 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();  // In Celsius

  // Check if the readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT read failed. Skipping this cycle.");
    return;  // Skip sending data
  }

  // Debugging: Print temperature and humidity to serial
  Serial.print("Temp: "); Serial.print(temperature); Serial.print(" °C, ");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");

  // Read the MQ3 sensor (analog reading)
  int mq3Value = analogRead(MQ3_PIN);  // Read analog value from A0 (MQ3)
  float voltage = mq3Value * (3.3 / 1023.0);  // Convert the raw value to voltage

  // Debugging: Print MQ3 sensor value to serial
  Serial.print("MQ3 Sensor Value: "); Serial.print(mq3Value); 
  Serial.print(" (Voltage: "); Serial.print(voltage); Serial.println(" V)");

  // Create a JSON payload for ThingsBoard
  String payload = String("{\"temp\":") + String(temperature, 2) + 
                   ",\"humidity\":" + String(humidity, 2) + 
                   ",\"mq3\": " + String(mq3Value) + "}";

  // Publish to ThingsBoard
  if (millis() - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = millis();
    client.publish("v1/devices/me/telemetry", (char*)payload.c_str());
  }
}