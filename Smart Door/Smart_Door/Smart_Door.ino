#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// --- WiFi ---
const char* WIFI_SSID = "Diba";
const char* WIFI_PASS = "89718971";

// --- ThingsBoard ---
const char* TB_HOST   = "demo.thingsboard.io";  // or your TB server
const int   TB_PORT   = 1883;
const char* TB_TOKEN  = "GyOCC9fM2scU2fi2rqqM";   // from ThingsBoard device

WiFiClient espClient;
PubSubClient mqtt(espClient);

// --- RFID ---
#define SS_PIN   D8
#define RST_PIN  D4
#define LED_PIN  D1
#define BUZZER   D0
#define DOOR_MS  3000

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Allowed UIDs
const byte ALLOWED[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},
  {0x04, 0x3A, 0xB2, 0x19}
};
const size_t ALLOWED_COUNT = sizeof(ALLOWED) / sizeof(ALLOWED[0]);

// --- Functions ---
bool uidMatch(byte *uid, byte size) {
  if (size != 4) return false;
  for (size_t i = 0; i < ALLOWED_COUNT; i++) {
    bool ok = true;
    for (byte j = 0; j < 4; j++) {
      if (ALLOWED[i][j] != uid[j]) { ok = false; break; }
    }
    if (ok) return true;
  }
  return false;
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

void sendEvent(String uid, String outcome) {
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!mqtt.connected()) mqttConnect();

  String payload = "{";
  payload += "\"uid\":\"" + uid + "\",";
  payload += "\"outcome\":\"" + outcome + "\"";
  payload += "}";

  mqtt.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Sent to TB: " + payload);
}

void buzzOK()   { tone(BUZZER, 2000, 80); delay(100); tone(BUZZER, 2500, 60); }
void buzzFail() { tone(BUZZER, 400, 200); }

void indicateUnlock() {
  digitalWrite(LED_PIN, HIGH);
  buzzOK();
  delay(DOOR_MS);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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

  if (uidMatch(mfrc522.uid.uidByte, mfrc522.uid.size)) {
    Serial.println("Access GRANTED: " + uidHex);
    indicateUnlock();
    sendEvent(uidHex, "granted");
  } else {
    Serial.println("Access DENIED: " + uidHex);
    buzzFail();
    sendEvent(uidHex, "denied");
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
// 🔹 LED
// Anode (+, long leg) → D1 (GPIO5) via 220 Ω resistor
// Cathode (–, short leg) → GND
// 🔹 Buzzer
// Positive (+) → D0 (GPIO16)
// Negative (–) → GND