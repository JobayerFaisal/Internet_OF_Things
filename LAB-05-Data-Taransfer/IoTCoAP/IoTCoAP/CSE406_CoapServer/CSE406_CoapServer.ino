#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
//#define COAP_RESPONSE_CONTENT  69  // 2.05 Content in CoAP (example)
//#define COAP_CONTENT_APPLICATION_OCTET_STREAM  42  // Example value
const char* ssid     = "ssid";
const char* password = "password";

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port);

// CoAP server endpoint url callback
void callback_light(CoapPacket &packet, IPAddress ip, int port);

// UDP and CoAP class
// other initialize is "Coap coap(Udp, 512);"
// 2nd default parameter is COAP_BUF_MAX_SIZE(defaulit:128)
// For UDP fragmentation, it is good to set the maximum under
// 1280byte when using the internet connection.
WiFiUDP udp;
Coap coap(udp);

// LED STATE
bool LEDSTATE;

// CoAP server endpoint URL
void callback_light(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Light] ON/OFF");
  
  // send response
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0';
  
  String message(p);
  Serial.println(p);
  if (message.equals("0"))
    LEDSTATE = false;
  else if(message.equals("1"))
    LEDSTATE = true;
  Serial.print("packet.token: ");
for (int i = 0; i < packet.tokenlen; i++) {
    Serial.print(packet.token[i], HEX);
    Serial.print(" ");
}
Serial.println();
//uint16_t sendResponse(IPAddress ip, int port, uint16_t messageid, const char *payload, size_t payloadlen, COAP_RESPONSE_CODE code, COAP_CONTENT_TYPE type, const uint8_t *token, int tokenlen);
  Serial.printf("packet.token: %s\n", packet.token);   
  if (LEDSTATE) {
    //digitalWrite(LED_BUILTIN, HIGH) ; 
    coap.sendResponse(ip, port, packet.messageid,  "1",1,COAP_CHANGED, COAP_APPLICATION_OCTET_STREAM , packet.token, packet.tokenlen);
    //coap.sendResponse(ip, port, packet.messageid,  "1",1,COAP_RESPONSE_CONTENT, COAP_CONTENT_APPLICATION_OCTET_STREAM , packet.token, packet.tokenlen);
  } else { 
   // digitalWrite(LED_BUILTIN, LOW) ; 
   coap.sendResponse(ip, port, packet.messageid,  "0",1,COAP_CHANGED, COAP_APPLICATION_OCTET_STREAM , packet.token, packet.tokenlen);
    //coap.sendResponse(ip, port, packet.messageid, "0",1);
  }
}

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Coap Response got]");
  
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  
  Serial.println(p);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Before Led initi");
  // LED State
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  LEDSTATE = true;
  Serial.println("After Led initi");
  // add server url endpoints.
  // can add multiple endpoint urls.
  // exp) coap.server(callback_switch, "switch");
  //      coap.server(callback_env, "env/temp");
  //      coap.server(callback_env, "env/humidity");
  Serial.println("Setup Callback Light");
  coap.server(callback_light, "light");

  // client response callback.
  // this endpoint is single callback.
  Serial.println("Setup Response Callback");
  coap.response(callback_response);

  // start coap server/client
  coap.start();
}

void loop() {
  delay(1000);
  coap.loop();
}
/*
if you change LED, req/res test with coap-client(libcoap), run following.
coap-client -m get coap://(arduino ip addr)/light
coap-client -e "1" -m put coap://(arduino ip addr)/light
coap-client -e "0" -m put coap://(arduino ip addr)/light
*/