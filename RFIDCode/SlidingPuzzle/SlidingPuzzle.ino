
/**
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read data from more than one PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to read data from more than one PICC (that is: a RFID Tag or Card) using a
   MFRC522 based RFID Reader on the Arduino SPI interface.

   Warning: This may not work! Multiple devices at one SPI are difficult and cause many trouble!! Engineering skill
            and knowledge are required!

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS 1    SDA(SS)      ** custom, take a unused pin, only HIGH/LOW required *
   SPI SS 2    SDA(SS)      ** custom, take a unused pin, only HIGH/LOW required *
   SPI MOSI    MOSI         11 / ICSP-4   51        41        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        42        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        43        ICSP-3           15

   More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout

*/
#include <PubSubClient.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

boolean mqttRst = false;
boolean mqttSolve = false;

const char* ssid = "Control booth";
const char* password = "MontyLives";
const char* mqtt_server = "192.168.86.101";
#define mqtt_port 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_SERIAL_PUBLISH_CH "/icircuit/ESP32/serialdata/tx"
#define MQTT_SERIAL_RECEIVER_CH "/icircuit/ESP32/serialdata/rx"

WiFiClient wifiClient;

PubSubClient client(wifiClient);

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_1_PIN        4         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define SS_2_PIN        5          // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 1
#define SS_3_PIN        12
#define SS_4_PIN        13
#define SS_5_PIN        14
#define SS_6_PIN        15
#define SS_7_PIN        16
//#define SS_8_PIN        17
//#define SS_9_PIN        25

#define NR_OF_READERS   7

byte ssPins[] = {SS_1_PIN, SS_2_PIN, SS_3_PIN, SS_4_PIN, SS_5_PIN, SS_6_PIN, SS_7_PIN/*, SS_8_PIN, SS_9_PIN*/};

MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.
int tagID;
int inputOrder[NR_OF_READERS];
int counter = 0;

const int correctOrder[9] = {1, 2, 3,
                             4, 5, 6,
                             7
                            };
//The "correct" sequence, default starts at 1 and counts up. This is the value stored in the first bit of the 5th block on the NFC tags.


/*
   Initialize.
*/

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
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("Egypt/Tomb/", "Slide puzzle online");
      // ... and resubscribe
      client.subscribe("Egypt/Tomb/slidePuzzle");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte * payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;
  Serial.println("-------new message from broker-----");
  Serial.print("channel:");
  Serial.println(topic);
  Serial.print("data:");
  Serial.write(payload, length);
  Serial.println();
  if (message == "ResetPuzzle") {
    mqttRst = true;
  }
  else if (message == "SolvePuzzle") {
    mqttSolve = true;
  }
}

void publishSerialData(char *serialData) {
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_SERIAL_PUBLISH_CH, serialData);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out for
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();

  SPI.begin();        // Init SPI bus

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
    inputOrder[reader] = 0;
  }
}

/*
   Main loop.
*/
void loop() {
  Serial.println(scan(1));
  delay(2000);
}


//Helper routine to dump a byte array as hex values to Serial.

void dump_byte_array(byte * buffer, byte bufferSize) {
  tagID = buffer[0];
}

int scan(uint8_t reader) {
  byte sector         = 1;
  byte blockAddr      = 5;
  byte dataBlock[]    = {
    0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
    0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
    0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
    0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
  };
  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  // Look for new cards

  delay(10); //allow bus to settle

  mfrc522[reader].PCD_Init();

  if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
    status = (MFRC522::StatusCode) mfrc522[reader].MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522[reader].GetStatusCodeName(status));
      return 0;
    }
    //Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 8); //Serial.println();
    /*Serial.print("Reader ");
      Serial.print(reader);
      Serial.print(": ");
      Serial.println(tagID);*/

    mfrc522[reader].PICC_HaltA();
    mfrc522[reader].PCD_StopCrypto1();

  } //if (mfrc522[reader].PICC_IsNewC

  return tagID;
}
