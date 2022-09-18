/*
   Initial Author: ryand1011 (https://github.com/ryand1011)

   Reads data written by a program such as "rfid_write_personal_data.ino"

   See: https://github.com/miguelbalboa/rfid/tree/master/examples/rfid_write_personal_data

   Uses MIFARE RFID card using RFID-RC522 reader
   Uses MFRC522 - Library
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15

   More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
*/

#include <SPI.h>
#include <MFRC522.h>
#include <FastLED.h>

#define RST_PINa         22           // Configurable, see typical pin layout above
#define SS_PINa          5          // Configurable, see typical pin layout above
#define RST_PINb         25           // Configurable, see typical pin layout above
#define SS_PINb          4          // Configurable, see typical pin layout above
#define DATA_PIN    14
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    8
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120

MFRC522 mfrc522a(SS_PINa, RST_PINa);   // Create MFRC522 instance
MFRC522 mfrc522b(SS_PINb, RST_PINb);

//*****************************************************************************************//
void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.begin(115200);                                           // Initialize serial communications with the PC
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                              // Init MFRC522 card
  Serial.println(F("Read personal data on a MIFARE PICC:"));    //shows in serial that it is ready to read
}

//*****************************************************************************************//
void loop() {
  leds[0] = CRGB(0, 0, 0);
  FastLED.show();
  if (mfrc522.PICC_IsNewCardPresent()) {
    int uid = getID();
    if (uid != -1) {
      Serial.print("Card detected, UID: "); Serial.println(uid);
    }

    if (uid == 1082416155) {
      leds[0] = CRGB(0, 255, 0);
      FastLED.show();
      FastLED.delay(250);
      Serial.println("Unlocked");
    }
    else {
      leds[0] = CRGB(255, 0, 0);
      FastLED.show();
      FastLED.delay(250);
    }
  }


}

//*****************************************************************************************//
unsigned long getID() {
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return -1;
  }
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  return hex_num;
}
