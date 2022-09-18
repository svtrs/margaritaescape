#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "Control booth"
#define STAPSK  "MontyLives"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;
const char* mqtt_server = "192.168.86.101";

#define mqtt_port 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_SERIAL_PUBLISH_CH "/icircuit/ESP32/serialdata/tx"
#define MQTT_SERIAL_RECEIVER_CH "/icircuit/ESP32/serialdata/rx"

WiFiClient wifiClient;

PubSubClient client(wifiClient); 

void setup_wifi() {  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/Egypt/Tomb/", "Tomb Door (Archive Entrance) Online");
      // ... and resubscribe
      client.subscribe("/Egypt/Tomb/Entrance/");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte *payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;
  Serial.println("-------new message from broker-----");
  Serial.print("channel:");
  Serial.println(topic);
  Serial.print("data:");
  Serial.write(payload, length);
  Serial.println();
  if (message == "open") {
    digitalWrite(D5, LOW);
    digitalWrite(D6, LOW);
    checkConnection();
    client.publish("/Egypt/Tomb/","Entrance Opened");
  }
  else if (message == "close"){
    digitalWrite(D5, HIGH);
    digitalWrite(D6, HIGH);
    checkConnection();
    client.publish("/Egypt/Tomb/","Entrance Closed");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  digitalWrite(D5, HIGH);
  digitalWrite(D6,HIGH);
  Serial.setTimeout(500);// Set time out for
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}

void checkConnection() {
  if (!client.connected()) {
    Serial.println("Lost connection");
    reconnect();
  }
}
void loop() {
  checkConnection();
  client.loop();
}
