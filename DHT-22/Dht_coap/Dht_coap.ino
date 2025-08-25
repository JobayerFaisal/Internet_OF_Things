// ESP8266 + DHT22 -> ThingsBoard over CoAP
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include <coap-simple.h>   // Library: "CoAP simple library" (hirotakaster)

// ---- USER SETTINGS ----
const char* WIFI_SSID     = "Diba";
const char* WIFI_PASSWORD = "89718971";

// Use your PC/server LAN IP for local TB (not "localhost")
IPAddress TB_IP(192,168,0,7);     // <-- CHANGE THIS to your TB host IP
const uint16_t COAP_PORT = 5683;   // CoAP UDP port
const char* ACCESS_TOKEN = "V3zQzSoVh7U0Be1JsiVD";

// Sensor pins
#define DHTPIN  D4      // GPIO2 (match your wiring)
#define DHTTYPE DHT22
const unsigned long PUBLISH_MS = 5000;
// -----------------------

WiFiUDP udp;
Coap coap(udp);
DHT dht(DHTPIN, DHTTYPE);

String coapPath;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();
  connectWiFi();

  // Build path: /api/v1/<TOKEN>/telemetry
  coapPath = String("api/v1/") + ACCESS_TOKEN + "/telemetry";

  // Start CoAP client
  coap.start();           // initialize internal state
  // Optional: set a response callback if you care about ACKs
  // coap.response([](coapPacket &pkt, IPAddress ip, int port){ Serial.println("CoAP ACK"); });
}

unsigned long lastSend = 0;

void loop() {
  // keep Wi-Fi alive
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  // CoAP housekeeping (handles ACKs/timeouts)
  coap.loop();

  unsigned long now = millis();
  if (now - lastSend < PUBLISH_MS) return;
  lastSend = now;

  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celsius
  if (isnan(h) || isnan(t)) {
    Serial.println("DHT read failed");
    return;
  }

  // JSON payload: {"temp": 27.51, "humidity": 63.20}
  String json = String("{\"temp\":") + String(t,2) + ",\"humidity\":" + String(h,2) + "}";
  Serial.println("POST " + coapPath + " -> " + json);

  // Send as CoAP POST (Confirmable by default). Path is relative (no leading slash).
  int msgId = coap.post(TB_IP, COAP_PORT, coapPath.c_str(),
                        (uint8_t*)json.c_str(), json.length());

  // If you want client-side timestamp instead:
  // unsigned long ts = (unsigned long)(time(nullptr)) * 1000UL; // needs NTP for real time
  // String jsonTS = String("{\"ts\":") + ts + ",\"values\":" + json + "}";
  // coap.post(TB_IP, COAP_PORT, coapPath.c_str(), (uint8_t*)jsonTS.c_str(), jsonTS.length());
}
