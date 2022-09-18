#include <PubSubClient.h>
#include <WiFi.h>

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

#define Door 19 //pin numbers to use for controlling relays   
#define Drawer 21
#define Pusher 5
#define Relic_Slide 4

#define Limit_Switch 33


WiFiClient wifiClient;

PubSubClient client(wifiClient);

void setup_wifi() {
  Serial.println();
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
    String clientId = "ESP32-Tomb-Exit";
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/Egypt/Tomb/", "Tomb Exit Online");
      // ... and resubscribe
      client.subscribe("/Egypt/Tomb/Exit/");
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
  if (message == "opendoor") {
    digitalWrite(Door, LOW);

    client.publish("/Egypt/Tomb/", "Exit Opened");
  }
  else if (message == "closedoor") {
    digitalWrite(Door, HIGH);

    client.publish("/Egypt/Tomb/", "Exit Closed");
  }
  else if (message == "opendrawer") {
    digitalWrite(Drawer, LOW);
    Serial.println("Drawer opened");
    client.publish("/Egypt/Tomb/", "Drawer opened");
  }
  else if (message == "closedrawer") {
    digitalWrite(Drawer, HIGH);

    client.publish("/Egypt/Tomb/", "Drawer closed");
  }
  else if (message == "pushrelic") {
    digitalWrite(Door, LOW);

    client.publish("/Egypt/Tomb/", "Retrival pusher actuated");
  }
  else if (message == "retractrelic") {
    digitalWrite(Door, HIGH);

    client.publish("/Egypt/Tomb/", "Retrival pusher retracted");
  }
  else if (message == "pushslide") {
    digitalWrite(Relic_Slide, LOW);

    client.publish("/Egypt/Tomb/", "Relic slide closed");
  }
  else if (message == "pullslide") {
    digitalWrite(Relic_Slide, HIGH);

    client.publish("/Egypt/Tomb/", "Relic slide opened");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(Door, OUTPUT);
  pinMode(Drawer, OUTPUT);
  pinMode(Relic_Slide, OUTPUT);
  pinMode(Pusher, OUTPUT);
  pinMode(Limit_Switch, INPUT_PULLUP);

  digitalWrite(Door, HIGH);
  digitalWrite(Drawer, HIGH);
  digitalWrite(Relic_Slide, HIGH);
  digitalWrite(Pusher, HIGH);
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

void wait(uint16_t msWait)
{
  uint32_t start = millis();

  while ((millis() - start) < msWait)
  {
    client.loop();
    checkConnection();
  }
}

void loop() {
  if (digitalRead(Limit_Switch) == LOW) {
    wait(500);
    digitalWrite(Relic_Slide, LOW);
  }
  checkConnection();
  client.loop();
}
