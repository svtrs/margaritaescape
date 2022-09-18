#include <Servo.h>
Servo servo1;


// Use this example with the Adafruit Keypad products.
// You'll need to know the Product ID for your keypad.
// Here's a summary:
//   * PID3844 4x4 Matrix Keypad
//   * PID3845 3x4 Matrix Keypad
//   * PID1824 3x4 Phone-style Matrix Keypad
//   * PID1332 Membrane 1x4 Keypad
//   * PID419  Membrane 3x4 Matrix Keypad

#include "Adafruit_Keypad.h"
#include <WiFi.h>
#include <PubSubClient.h>
char* ssid = "Control booth";
const char* password = "MontyLives";
const char* mqtt_server = "192.168.86.101";
#define mqtt_port 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_SERIAL_PUBLISH_CH "/icircuit/ESP32/serialdata/tx"
#define MQTT_SERIAL_RECEIVER_CH "/icircuit/ESP32/serialdata/rx"

#define DrawerSolenoid 33
#define doorSolenoid 22
#define Relay 18

#define MAX_DIGITS 8 //maximum number of inputs for a sequence
#define servoPin 32
#define limitSwitch 19
#define limitSwitchDoor 21
char mqttTopicPrefix[32] = "/Egypt/Archives/keypadPuzzle/";
char mqttTopic[32];

WiFiClient wifiClient;

PubSubClient client(wifiClient);

// define your specific keypad here via PID
#define KEYPAD_PID3845
// define your pins here
// can ignore ones that don't apply
#define R1    5
#define R2    16
#define R3    15
#define R4    13
#define C1    12
#define C2    4
#define C3    14
//#define C4    11
// leave this import after the above configuration
#include "keypad_config.h"
int input = 0;
int inputCode[MAX_DIGITS] = {};

int state = 0;
int lastSwitchState = 0;

int lastState = 0;
boolean keyDone = false;

boolean killInstance = false;
boolean mrst = false;

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out for
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
  customKeypad.begin();
  rst(MAX_DIGITS);
  pinMode(DrawerSolenoid, OUTPUT);
  pinMode(doorSolenoid, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(limitSwitch, INPUT_PULLUP);
  pinMode(limitSwitchDoor, INPUT_PULLUP);
  digitalWrite(Relay, LOW);
  digitalWrite(DrawerSolenoid, LOW);
  digitalWrite(doorSolenoid, HIGH);
  servoWrite(70);
}

void checkConnection() {
  if (!client.connected()) {
    Serial.print("Lost connection");
    reconnect();
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
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
    String clientId = "ESP32-Keypad-Puzzle";
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/Egypt/Archives/", "Keypad puzzle online");
      // ... and resubscribe
      client.subscribe("/Egypt/Archives/keypadPuzzle/digitSelect/");
      client.subscribe("/Egypt/Archives/keypadPuzzle/");
      client.subscribe("servo");
      client.subscribe("Egypt/Library/");
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

  if (!strcmp(topic, mqttFullTopic("digitSelect/"))) {
    lastState = value;
    seekDigits(value);
    killInstance = true;
  }
  else if (message == "RFIDpuzzle Solved" || message == "speakeron") {
    digitalWrite(Relay, HIGH);
  }
  else if (message == "RFIDpuzzle Reset" || message == "speakeroff") {
    digitalWrite(Relay, LOW);
  }
  else if (message == "stop") {
    killInstance = true;
  }
  else if (message == "reset") {
    client.publish("/Egypt/Archives/", "Keypad reset. Reactivating.");
    killInstance = true;
    mrst = true;
  }
  else if (message == "opendrawer") {
    digitalWrite(DrawerSolenoid, HIGH);
  }
  else if (message == "closedrawer") {
    digitalWrite(DrawerSolenoid, LOW);
  }
  else if (message == "revealknob") {
    digitalWrite(doorSolenoid, LOW);
    while (true) {
      if(digitalRead(limitSwitchDoor) == LOW){
        client.loop();
        client.publish("/Egypt/Archives/","Knob Turned");
        Serial.println("Switch closed");
        break;
      }
    }
  }
  else if (message == "retractknob") {
    digitalWrite(doorSolenoid , HIGH);
  }
  else if (message == "dropkey") {
    servoWrite(35);
  }
  else if (message == "closekey") {
    servoWrite(70);
  }
  else if (!strcmp(topic, "servo")) {
    servoWrite(value);
  }
  else if (message == "watchdoor") {
    
  }
  checkConnection();

}

void rst(int digitLength) {
  for (int i = 0; i < digitLength; i++) {
    inputCode[i] = 10;
  }
}

char* mqttFullTopic(const char action[]) {
  strcpy (mqttTopic, mqttTopicPrefix);
  strcat (mqttTopic, action);
  return mqttTopic;
}

void seekDigits(int digitLength) {
  rst(digitLength);
  int finalValue = 0;
  killInstance = false;

  customKeypad.tick();
  client.loop();
  Serial.println("Enter Passcode");
  checkConnection();

  for (int i = 0; i < digitLength; i++) {
    while (inputCode[i] == 10) {
      client.loop();
      customKeypad.tick();
      lastState = state;
      state = digitalRead(limitSwitch);
      if (lastState != state) {
        killInstance = true;
      }
      if (killInstance == true) {
        return;
      }
      keypadEvent e = customKeypad.read();
      input = int((char)e.bit.KEY) - '0';
      String payload = String((char)e.bit.KEY);
      String iteration = String(i);
      iteration += "/";
      const char *channel = iteration.c_str();
      if (e.bit.EVENT == KEY_JUST_PRESSED) {
        inputCode[i] = input;
        if (input == -13 || input == -6) {
          inputCode[i] = 10;
          i--;
          client.publish(mqttFullTopic("Hash/"), (char*) payload.c_str());
        }
        else {
          client.publish(mqttFullTopic(channel), (char*) payload.c_str());
          Serial.println(inputCode[i]);
        }
      }
      wait(10);
    }
  }
  if (killInstance == false) {
    while (input != -13) {
      customKeypad.tick();
      keypadEvent e = customKeypad.read();
      input = int((char)e.bit.KEY) - '0';
      String payload = String((char)e.bit.KEY);
      client.loop();
    }


    for (int i = 0; i < digitLength; i++) {
      finalValue = finalValue + inputCode[i] * (pow(10, (digitLength - 1 - i)));
    }

    String finalValueString = String(finalValue);
    client.publish(mqttFullTopic("Result/"), (char*) finalValueString.c_str());
  }
  keyDone = true;
}

void servoWrite(int microseconds) {
  servo1.attach(servoPin);
  servo1.write(microseconds);
  wait(650);
  servo1.detach();
}

void wait(int ms) {
  for (int i = 0; i < ms; i++) {
    client.loop();
    delay(1);
  }
}

void loop() {
  client.loop();
  state = digitalRead(limitSwitch);

  while (state == 1) {
    client.loop();
    client.publish("/Egypt/Archives/", "4 digit mode, drawer closed");
    seekDigits(4);
    lastSwitchState = state;
    state = digitalRead(limitSwitch);
    if (lastSwitchState != state) {
      break;
    }
    while (mrst == false) {
      lastSwitchState = state;
      state = digitalRead(limitSwitch);
      if (lastSwitchState != state) {
        break;
      }
      wait(10);
    }
    mrst = false;
  }

  while (state == 0) {
    client.loop();
    client.publish("/Egypt/Archives/", "5 digit mode, drawer opened");
    seekDigits(5);
    lastSwitchState = state;
    state = digitalRead(limitSwitch);
    if (lastSwitchState != state) {
      break;
    }
    while (mrst == false) {
      lastSwitchState = state;
      state = digitalRead(limitSwitch);
      if (lastSwitchState != state) {
        break;
      }
      wait(10);
    }
    mrst = false;
  }






  /*
    state = digitalRead(limitSwitch);
    if ((state != lastSwitchState)) {
    Serial.println(state);
    if (state == 0) {
      client.publish("/Egypt/Archives/", "Drawer Closed");
      seekDigits(4);
    }
    else if (state == 1) {
      client.publish("/Egypt/Archives/", "Drawer Opened");
      seekDigits(5);
    }
    }
    lastSwitchState = state;
    keyDone = false;
    wait(10);
    checkConnection();*/
}
