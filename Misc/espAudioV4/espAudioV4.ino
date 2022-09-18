/***************************************************
  DFPlayer - A Mini MP3 Player For Arduino
  <https://www.dfrobot.com/index.php?route=product/product&product_id=1121>

 ***************************************************
  This example shows the all the function of library for DFPlayer.

  Created 2016-12-07
  By [Angelo qiao](Angelo.qiao@dfrobot.com)

  GNU Lesser General Public License.
  See <http://www.gnu.org/licenses/> for details.
  All above must be included in any redistribution
 ****************************************************/

/***********Notice and Trouble shooting***************
  1.Connection and Diagram can be found here
  <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>
  2.This code is tested on Arduino Uno, Leonardo, Mega boards.
 ****************************************************/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <PubSubClient.h>
#include<ESP8266WiFi.h>

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

char mqttTopicPrefix[64] = "/Egypt/Library/espAudio/Ambient/";
char mqttTopic[64];
int Folder = 1;

WiFiClient wifiClient;

PubSubClient client(wifiClient);

SoftwareSerial mySoftwareSerial(D2, D1); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  Serial.setTimeout(2000);// Set time out for
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
  Serial.println("initializing...");

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    client.publish("/Egypt/Library/", "Library SFX error, check SD");
    while (true);
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms

  //----Set volume----
  myDFPlayer.volume(10);  //Set volume value (0~30).
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  client.publish("/Egypt/Library/", "Library SFX ready");
}


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/Egypt/Library/", "Library SFX connected. Loading audio");
      // ... and resubscribeclient.subscribe(mqttFullTopic(""));
      client.subscribe(mqttFullTopic("Play/"));
      client.subscribe(mqttFullTopic("Pause/"));
      client.subscribe(mqttFullTopic("Resume/"));
      client.subscribe(mqttFullTopic("Stop/"));
      client.subscribe(mqttFullTopic("Volume/"));
      client.subscribe(mqttFullTopic("Interrupt/"));
      client.subscribe(mqttFullTopic("Folder/"));
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
  int value = message.toInt();
  Serial.println("-------new message from broker-----");
  Serial.print("channel:");
  Serial.println(topic);
  Serial.print("data:");
  Serial.write(payload, length);
  Serial.println();

  if (!strcmp(topic, mqttFullTopic("Play/"))) {
    myDFPlayer.playFolder(Folder,value); //sd:/01/track
  }
  else if (!strcmp(topic, mqttFullTopic("Pause/"))) {
    myDFPlayer.pause();
  }
  else if (!strcmp(topic, mqttFullTopic("Resume/"))) {
    myDFPlayer.start();
  }
  else if (!strcmp(topic, mqttFullTopic("Stop/"))) {
    myDFPlayer.stop();
  }
  else if (!strcmp(topic, mqttFullTopic("Volume/"))) {
    myDFPlayer.volume(value);
  }
  else if (!strcmp(topic, mqttFullTopic("Folder/"))) {
    Folder = value;
  }
  else if (!strcmp(topic, mqttFullTopic("Interrupt/")) & message != "Stop") {
    myDFPlayer.advertise(value); //sd:/advert/track
  }
  else if (!strcmp(topic, mqttFullTopic("Interrupt/")) & message == "Stop") {
    myDFPlayer.stopAdvertise();
  }
}

void checkConnection() {
  if (!client.connected()) {
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
    delay(1);
  }
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

char* mqttFullTopic(const char action[]) {
  strcpy (mqttTopic, mqttTopicPrefix);
  strcat (mqttTopic, action);
  return mqttTopic;
}

void loop() {
  client.loop();
  checkConnection();
}
