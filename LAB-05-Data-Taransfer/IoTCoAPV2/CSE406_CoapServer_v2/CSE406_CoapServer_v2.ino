#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

const char *ssid = "Diba";
const char *password = "89718971";

void callback_response(CoapPacket &packet, IPAddress ip, int port);
void callback_light(CoapPacket &packet, IPAddress ip, int port);

WiFiUDP udp;
Coap coap(udp);

// This variable will track the logical state (true=ON, false=OFF)
bool ledIsOn;

// CoAP server endpoint URL
void callback_light(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("\n[Light] Request received.");

  // Create a null-terminated string from the payload
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0'; // Use the null character '\0'

  String message(p);
  Serial.print("Payload received: ");
  Serial.println(message);

  // --- CORRECTED LOGIC ---
  if (message.equals("1")) {
    Serial.println("Instruction: Turn ON");
    digitalWrite(LED_BUILTIN, LOW); // LOW turns the active-low LED ON
    ledIsOn = true;
  } else if (message.equals("0")) {
    Serial.println("Instruction: Turn OFF");
    digitalWrite(LED_BUILTIN, HIGH); // HIGH turns the active-low LED OFF
    ledIsOn = false;
  }

  // Send a response back to the client
  if (ledIsOn) {
    coap.sendResponse(ip, port, packet.messageid, "1", 1, COAP_CHANGED, COAP_APPLICATION_OCTET_STREAM, packet.token, packet.tokenlen);
  } else {
    coap.sendResponse(ip, port, packet.messageid, "0", 1, COAP_CHANGED, COAP_APPLICATION_OCTET_STREAM, packet.token, packet.tokenlen);
  }
}

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Coap Response got]");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0'; // CORRECTED: Use '\0' not NULL

  Serial.println(p);
}


void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nBooting...");

  // Setup the built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Start with the LED OFF
  ledIsOn = false;                 // Set initial state to OFF

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP().toString());

  // Setup CoAP server
  coap.server(callback_light, "light");
  coap.response(callback_response);
  coap.start();
  Serial.println("CoAP Server Started.");
}

void loop() {
  coap.loop();
  // We can add a small delay to prevent the loop from running too fast
  delay(100);
}