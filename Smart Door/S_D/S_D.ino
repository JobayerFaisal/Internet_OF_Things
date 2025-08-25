#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>

/********* WiFi *********/
const char* WIFI_SSID = "Diba";
const char* WIFI_PASS = "89718971";

/********* NTP (Asia/Dhaka UTC+6) *********/
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 6 * 3600;
const int   DAYLIGHT_OFFSET_SEC = 0;

/********* ThingsBoard (MQTT) *********/
const char* TB_HOST   = "demo.thingsboard.io";  // or your TB server/IP
const int   TB_PORT   = 1883;
const char* TB_TOKEN  = "GyOCC9fM2scU2fi2rqqM";    // from TB device
WiFiClient espClient;
PubSubClient mqtt(espClient);

/********* RFID pins & IO *********/
#define SS_PIN    D8   // RC522 SDA
#define RST_PIN   D4   // RC522 RST
#define LED_PIN   D1   // LED anode via 220Ω -> D1, cathode -> GND
#define BUZZER    D0   // passive piezo buzzer +
#define UNLOCK_MS 2000
#define COOLDOWN_MS 800

MFRC522 mfrc522(SS_PIN, RST_PIN);

/********* Users (UID -> name -> personal tone) *********/
struct User {
  byte uid[4];
  const char* name;
  int toneHz;
};

User USERS[] = {
  {{0xDE,0xAD,0xBE,0xEF}, "Alice", 1600},
  {{0x04,0x3A,0xB2,0x19}, "Bob",   1900}
};
const size_t USER_COUNT = sizeof(USERS)/sizeof(USERS[0]);

/********* Helpers *********/
bool matchUID(const byte* uid, byte size, int &userIndex) {
  if (size != 4) { userIndex = -1; return false; }
  for (size_t i=0; i<USER_COUNT; ++i) {
    bool ok = true;
    for (int j=0; j<4; ++j) if (USERS[i].uid[j] != uid[j]) { ok = false; break; }
    if (ok) { userIndex = (int)i; return true; }
  }
  userIndex = -1;
  return false;
}

String uidHexStr(const byte* uid, byte size, const char* sep=":") {
  String s;
  for (byte i=0;i<size;i++) {
    if (i) s += sep;
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

String nowString() {
  time_t now = time(nullptr);
  if (now < 8) return ""; // if NTP not yet synced
  struct tm *tm_info = localtime(&now);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
  return String(buf);
}

/********* WiFi / MQTT / NTP *********/
void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println(" ✓");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}

void mqttEnsure() {
  mqtt.setServer(TB_HOST, TB_PORT);
  while (!mqtt.connected()) {
    Serial.print("TB MQTT...");
    if (mqtt.connect("rfid-door", TB_TOKEN, nullptr)) Serial.println("connected");
    else { Serial.printf("fail rc=%d\n", mqtt.state()); delay(1500); }
  }
}

/********* Buzzer patterns *********/
void buzzGranted() { tone(BUZZER, 2000, 120); delay(150); }
void buzzDenied()  { tone(BUZZER,  600, 600); delay(650); }
void buzzUserSignature(int baseHz) {
  tone(BUZZER, baseHz,     120); delay(150);
  tone(BUZZER, baseHz+250, 120); delay(150);
  tone(BUZZER, baseHz+400, 160); delay(200);
}

/********* LED indication *********/
void indicateGranted() {
  digitalWrite(LED_PIN, HIGH);
  buzzGranted();
  delay(UNLOCK_MS);
  digitalWrite(LED_PIN, LOW);
}

/********* Send to ThingsBoard *********/
void tbSendEvent(const String& uidHex, const String& outcome, const String& who) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqtt.connected()) mqttEnsure();
  String ts = nowString(); // may be empty if NTP not ready
  String payload = "{\"uid\":\"" + uidHex + "\",\"outcome\":\"" + outcome + "\",\"user\":\"" + who + "\"";
  if (ts.length()) payload += ",\"ts\":\"" + ts + "\"";
  payload += "}";
  mqtt.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("TB >> " + payload);
}

/********* Setup / Loop *********/
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  wifiConnect();
  mqtt.setServer(TB_HOST, TB_PORT);

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID + Buzzer + ThingsBoard ready. Tap a card...");
}

void loop() {
  mqtt.loop();  // keep MQTT alive

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return;

  int userIndex = -1;
  bool allowed = matchUID(mfrc522.uid.uidByte, mfrc522.uid.size, userIndex);
  String uidStr = uidHexStr(mfrc522.uid.uidByte, mfrc522.uid.size);

  if (allowed) {
    const char* who = USERS[userIndex].name;
    Serial.printf("[GRANTED] %s (%s)\n", who, uidStr.c_str());
    buzzUserSignature(USERS[userIndex].toneHz);
    indicateGranted();
    tbSendEvent(uidStr, "granted", who);
  } else {
    Serial.printf("[DENIED] %s\n", uidStr.c_str());
    buzzDenied();
    tbSendEvent(uidStr, "denied", "unknown");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(COOLDOWN_MS);
}


// RC522 SDA	D8	GPIO15
// RC522 SCK	D5	GPIO14
// RC522 MOSI	D7	GPIO13
// RC522 MISO	D6	GPIO12
// RC522 RST	D4	GPIO2
// RC522 VCC	3V3	3.3V
// RC522 GND	G	GND
// LED (+)	D1 (via 220 Ω)	GPIO5
// LED (–)	G	GND
// Buzzer (+)	D0	GPIO16
// Buzzer (–)	G	GND
