
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

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_1_PIN        5         // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 2
#define SS_2_PIN        17          // Configurable, take a unused pin, only HIGH/LOW required, must be different to SS 1
#define SS_3_PIN        16

#define NR_OF_READERS   3

byte ssPins[] = {SS_1_PIN, SS_2_PIN, SS_3_PIN};

MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.

int tagID;
int inputOrder[3] = {0, 0, 0};
const int correctOrder[3] = {1, 2, 3};
int counter = 0;

/*
   Initialize.
*/
void setup() {

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Serial.begin(115200); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  SPI.begin();        // Init SPI bus

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
    delay(100);
  }

}

/*
   Main loop.
*/
void loop() {

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  boolean solved = true ;
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

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    // Look for new cards

    delay(10); //allow bus to settle

    mfrc522[reader].PCD_Init();

    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      status = (MFRC522::StatusCode) mfrc522[reader].MIFARE_Read(blockAddr, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522[reader].GetStatusCodeName(status));
      }
      //Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
      dump_byte_array(buffer, 8); //Serial.println();
      Serial.print("Reader ");
      Serial.print(reader);
      Serial.print(": ");
      Serial.println(tagID);
      inputOrder[reader] = tagID;
      Serial.println(inputOrder[reader]);

      for (int i = 0; i < 3; i++) {
        Serial.print(inputOrder[i]);
        Serial.print(" ");
      }

      mfrc522[reader].PICC_HaltA();
      mfrc522[reader].PCD_StopCrypto1();


    } //if (mfrc522[reader].PICC_IsNewC
    else {
      if (counter % 3 == 0) {
        inputOrder[reader] = 0;
      }
    }
    if (inputOrder[reader] != correctOrder[reader]) {
      solved = false;
    }

  } //for(uint8_t reader


  if (solved == true) {
    digitalWrite(4, LOW);
    /*while (true) {
      Serial.print("bungus");
      for (int i = 0; i < 3; i++) {
      inputOrder[i] = 0;
      }*/
    for (int i = 0; i < 3; i++) {
      inputOrder[i] = 0;
      Serial.print("SOLVED");
    }

    delay(3000);
  }


  counter = counter + 1;
}


//Helper routine to dump a byte array as hex values to Serial.

void dump_byte_array(byte *buffer, byte bufferSize) {
  tagID = buffer[0];
}
