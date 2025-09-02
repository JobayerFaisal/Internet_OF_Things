#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// --- WiFi ---
const char* WIFI_SSID = "Jobayer";
const char* WIFI_PASS = "12345678";

// --- ThingsBoard ---
// const char* TB_HOST   = "192.168.0.106";
// const char* TB_TOKEN  = "abHynoUovzfgBIeXBld9";

const char* TB_HOST   = "demo.thingsboard.io";  // or your TB server
const int   TB_PORT   = 1883;
const char* TB_TOKEN  = "GyOCC9fM2scU2fi2rqqM";   // from ThingsBoard device

WiFiClient espClient;
PubSubClient mqtt(espClient);

// --- RFID ---
#define SS_PIN   D8
#define RST_PIN  D4
#define GREEN_LED_PIN  D1
#define RED_LED_PIN    D2
#define BUZZER   D0
#define DOOR_MS  2000

MFRC522 mfrc522(SS_PIN, RST_PIN);

struct User {
  byte uid[4];
  String username;
};

// Create an array of Users
User allowedUsers[] = {
  {{0xD9, 0x4A, 0xC8, 0x01}, "DINA"},
  {{0xA5, 0x1E, 0x28, 0x02}, "DISHA"}
};

const size_t ALLOWED_COUNT = sizeof(allowedUsers) / sizeof(allowedUsers[0]);


// --- Functions ---
String uidMatch(byte *uid, byte size) {
  if (size != 4) return "";  // If the UID size is incorrect, return empty string

  for (size_t i = 0; i < ALLOWED_COUNT; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (allowedUsers[i].uid[j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return allowedUsers[i].username; // Return the username if UID matches
    }
  }
  return ""; // Return empty string if no match is found
}


void wifiConnect() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" connected!");
}

void mqttConnect() {
  mqtt.setServer(TB_HOST, TB_PORT);
  while (!mqtt.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (mqtt.connect("rfid-client", TB_TOKEN, NULL)) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying in 2s");
      delay(2000);
    }
  }
}

void sendDoorCloseEvent(String uid, String username) {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!mqtt.connected()) mqttConnect();

  String payload = "{";
  payload += "\"uid\":\"" + uid + "\",";
  payload += "\"username\":\"" + username + "\",";
  payload += "\"door_status\":\"closed\",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";

  mqtt.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Door closed event sent to TB: " + payload);
}

void sendEvent(String uid, String outcome, String username) {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!mqtt.connected()) mqttConnect();

  // Determine door status
  String door_status = (outcome == "granted") ? "open" : "closed";

  String payload = "{";
  payload += "\"uid\":\"" + uid + "\",";
  payload += "\"outcome\":\"" + outcome + "\",";
  payload += "\"username\":\"" + username + "\",";
  payload += "\"door_status\":\"" + door_status + "\",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";

  // Publish as Timeseries data
  mqtt.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Sent to TB: " + payload);

  // If access is granted, automatically close door after 10s
  if (outcome == "granted") {
    delay(10000);  // Wait 10 seconds
    sendDoorCloseEvent(uid, username);
  }
}



void buzzOK()   { 
  tone(BUZZER, 2000, 80); 
  delay(100); 
  tone(BUZZER, 2500, 60); }

void buzzFail() {
  tone(BUZZER, 400, 200); // Play failure tone for 200ms
  delay(200);              // Wait for 200ms
  tone(BUZZER, 400, 200); // Play failure tone again for 200ms
  delay(200);              // Wait for 200ms
  tone(BUZZER, 400, 200); // Play failure tone again for 200ms
  delay(200);              // Wait for 200ms
  noTone(BUZZER);          // Stop the buzzer after 2 seconds
}

void indicateDenied() {
  digitalWrite(RED_LED_PIN, HIGH); // Turn on the red LED
  buzzFail(); // Play the failure tone
  delay(DOOR_MS);
  digitalWrite(RED_LED_PIN, LOW); // Turn off the red LED after the delay
}

void indicateUnlock() {
  digitalWrite(GREEN_LED_PIN, HIGH);
  buzzOK();
  delay(DOOR_MS);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);

  wifiConnect();
  mqttConnect();

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID + ThingsBoard ready...");
}

void loop() {
  mqtt.loop(); // keep MQTT alive

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return;

  // Build UID string
  String uidHex = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (i) uidHex += ":";
    if (mfrc522.uid.uidByte[i] < 0x10) uidHex += "0";
    uidHex += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidHex.toUpperCase();

if (uidMatch(mfrc522.uid.uidByte, mfrc522.uid.size) != "") {
  String username = uidMatch(mfrc522.uid.uidByte, mfrc522.uid.size); // Fetch username
  Serial.println("Access GRANTED: " + uidHex + " Username: " + username);
  indicateUnlock();
  sendEvent(uidHex, "granted", username); // Pass username to sendEvent
} else {
  Serial.println("Access DENIED: " + uidHex);
  indicateDenied();
  sendEvent(uidHex, "denied", ""); // No username for denied access
}

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}



//MFRC522 by Miguel Balboa
// RFID  
// SDA (SS) → D8 (GPIO15)
// SCK → D5 (GPIO14)
// MOSI → D7 (GPIO13)
// MISO → D6 (GPIO12)
// RST → D4 (GPIO2)
// 3.3V → 3V3 (⚠️ Only 3.3 V, never 5 V)
// GND → G
// 🔹 GREEN_LED
// Anode (+, long leg) → D1 (GPIO5) via 220 Ω resistor
// Cathode (–, short leg) → GND
// 🔹 RED_LED
// Anode (+, long leg) → D2 (GPIO7) via 220 Ω resistor
// Cathode (–, short leg) → GND
// 🔹 Buzzer
// Positive (+) → D0 (GPIO16)
// Negative (–) → GND
