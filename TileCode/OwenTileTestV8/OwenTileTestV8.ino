#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>


// Update these with values suitable for your network.
const char* ssid = "Control booth";
const char* password = "MontyLives";
const char* mqtt_server = "192.168.86.101";
#define mqtt_port 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_SERIAL_PUBLISH_CH "/icircuit/ESP32/serialdata/tx"
#define MQTT_SERIAL_RECEIVER_CH "/icircuit/ESP32/serialdata/rx"
#define MQTT_MSG_SIZE 256
WiFiClient wifiClient;
PubSubClient client(wifiClient);

char mqttTopicPrefix[32] = "/Egypt/Tomb/Tiles/";
char mqttTopic[MQTT_MSG_SIZE];

boolean mqttSimonSolve = false;
int mqttSimonDifficulty = 1;
boolean simonSolved = false;

boolean mqttStepPuzzleSolve = false;
boolean spSolved = false;

boolean mqttDemoModeStart = false;
boolean mqttDemoModeEnd = false;

int playerCount = 2;
int simonDifficulty = 1;

//Mux control pins
int s0 = 27;
int s1 = 23;
int s2 = 22;
int s3 = 21;

//Mux in "SIG" pin
int SIG_pin0 = 32;
int SIG_pin1 = 33;

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    1144
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          80
#define FRAMES_PER_SECOND  120

const int startTile = 0;                                 //Tile number-1
const int endTile = 31;
int tileState [endTile + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //specify number of tiles in tileState[] and lastState
int lastState [endTile + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int nullRow = NUM_LEDS / 8;
const int tileRow [endTile + 1][4] = {   //tileRow[number of tiles][number of rows]
  //142.5 is a null row for tiles with 3 sides

  {12, 9, nullRow, 1}, //Tile 1
  {11, 8, 12, 2},     //Tile 2
  {10, 7, 11, 3},     //Tile 3
  {5, 6, 10, 4},      //Tile 4

  {13, 26, 27, 9},    //Tile 5
  {15, 24, 25, 14},   //Tile 6
  {17, 22, 23, 16},   //Tile 7
  {19, 20, 21, 18},   //Tile 8

  {27, 38, 35, nullRow}, //Tile 9
  {28, 37, 34, 38},   //Tile 10
  {29, 36, 33, 37},   //Tile 11
  {30, 31, 32, 36},   //Tile 12

  {35, 39, 52, 53},   //Tile 13
  {40, 41, 50, 51},   //Tile 14
  {42, 43, 48, 49},   //Tile 15
  {44, 45, 46, 47},   //Tile 16

  {nullRow, 53, 64, 61}, //Tile 17
  {64, 54, 63, 60},   //Tile 18
  {63, 55, 62, 59},   //Tile 19
  {62, 56, 57, 58},   //Tile 20

  {79, 61, 65, 78},   //Tile 21
  {77, 66, 67, 76},   //Tile 22
  {75, 68, 69, 74},   //Tile 23
  {73, 70, 71, 72},   //Tile 24

  {87, nullRow, 79, 90}, //Tile 25
  {86, 90, 80, 89},   //Tile 26
  {85, 89, 81, 88},   //Tile 27
  {84, 88, 82, 83},   //Tile 28

  {104, 1, 87, 91},   //Tile 29
  {102, 103, 92, 93}, //Tile 30
  {100, 101, 94, 95}, //Tile 31
  {98, 99, 96, 97},   //Tile 32

};

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
  // loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Tile ESP32";
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/Egypt/Tomb/", "Floor tiles online");
      // ... and resubscribe
      client.subscribe("/Egypt/Tomb/Tiles/");
      client.subscribe(mqttFullTopic("Simon/"));
      client.subscribe(mqttFullTopic("stepPuzzle/"));
      client.subscribe(mqttFullTopic("Simon/Rounds/"));
      client.subscribe(mqttFullTopic("stepPuzzle/playerCount/"));
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
  int val = message.toInt();
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("data:");
  Serial.write(payload, length);
  Serial.println();
  if (!strcmp(topic, mqttFullTopic("Simon/")) & message == "start") {
    closeEncounters();
    while (simonSolved == false) {
      simonMode(simonDifficulty);
      wait(2000);
    }
    simonSolved = false;
  }
  else if (!strcmp(topic, mqttFullTopic("Simon/")) & message == "solve") {
    mqttSimonSolve = true;
  }
  else if (!strcmp(topic, mqttFullTopic("stepPuzzle/")) & message == "start") {
    StepPuzzle(playerCount);
  }
  else if (!strcmp(topic, mqttFullTopic("stepPuzzle/")) & message == "solve") {
    mqttStepPuzzleSolve = true;
  }
  else if (!strcmp(topic, mqttFullTopic("Simon/Rounds/"))) {
    simonDifficulty = val;
    Serial.print("Simon Difficulty Set to ");
    Serial.print(val);
    checkConnection();
    client.publish("/Egypt/Tomb/", "Simon difficulty set");
  }
  else if (!strcmp(topic, mqttFullTopic("stepPuzzle/playerCount/"))) {
    playerCount = val;
    Serial.print("Player count set to ");
    Serial.print(val);
    checkConnection();
    client.publish("/Egypt/Tomb/", "Player count set");
  }



}


void setup() {
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);

  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);

  Serial.begin(115200);

  Serial.setTimeout(500);// Set time out for
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);


}

void checkConnection() {
  if (!client.connected()) {
    Serial.println("Lost connection");
    reconnect();
  }
}


void loop() {

  safetyLight();
  for (int i = -767; i < 767; i = i + 6) {
    client.loop();
    checkConnection();
    /* if (abs(i) >= 255 && abs(i) < 511) {
       FastLED.setBrightness(abs(i) - 255);
      }
      else if (abs(i) >= 511) {
       FastLED.setBrightness(255);
      }
      else if (abs(i) < 255) {
       FastLED.setBrightness(0);
      }*/

    FastLED.setBrightness( beatsin8(60, 128, 255));
    EVERY_N_MILLISECONDS(10) {
      pacifica_loop();
    }
    FastLED.show();

  }
}



//custom functions below

//                                                                                                          base level functions
//------------------------------------------------------------------------------------------------------------------------------
int readMux(int channel, int readPin) {                          //Adam Meyer's mux code. Has been modified to have a second parameter to specify which pin is used for reading
  int controlPin[] = {s0, s1, s2, s3};                            //on the ESP because we have 2 muxes.

  int muxChannel[16][4] = {
    {0, 0, 0, 0}, //channel 0
    {1, 0, 0, 0}, //channel 1
    {0, 1, 0, 0}, //channel 2
    {1, 1, 0, 0}, //channel 3
    {0, 0, 1, 0}, //channel 4
    {1, 0, 1, 0}, //channel 5
    {0, 1, 1, 0}, //channel 6
    {1, 1, 1, 0}, //channel 7
    {0, 0, 0, 1}, //channel 8
    {1, 0, 0, 1}, //channel 9
    {0, 1, 0, 1}, //channel 10
    {1, 1, 0, 1}, //channel 11
    {0, 0, 1, 1}, //channel 12
    {1, 0, 1, 1}, //channel 13
    {0, 1, 1, 1}, //channel 14
    {1, 1, 1, 1} //channel 15
  };

  //loop through the 4 sig
  for (int i = 0; i <= 3; i ++) {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  //read the value at the SIG pin
  int val = analogRead(readPin);

  //return the value
  return val;
}
//---------------------------------------------------------------

char* mqttFullTopic(const char action[]) {
  strcpy (mqttTopic, mqttTopicPrefix);
  strcat (mqttTopic, action);
  return mqttTopic;
}
//---------------------------------------------------------------
void wait(uint16_t msWait) {
  uint32_t start = millis();

  while ((millis() - start) < msWait)
  {
    client.loop();
    checkConnection();
    delay(1);
  }
}

//---------------------------------------------------------------
CRGBPalette16 pacifica_palette_1 =
{ 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
  0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50
};
CRGBPalette16 pacifica_palette_2 =
{ 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
  0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F
};
CRGBPalette16 pacifica_palette_3 =
{ 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
  0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF
};

void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 188);
  uint16_t speedfactor2 = beatsin16(4, 179, 188);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
  sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
  sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
  sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0 - beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10, 38), 0 - beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10, 28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 100;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs / 2;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 2 );

  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if ( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145);
    leds[i].green = scale8( leds[i].green, 200);
    leds[i] |= CRGB( 2, 5, 7);
  }
}
//---------------------------------------------------------------
void lightRow (int rowNum, int R, int G, int B) {                //lightRow lights the specified row by finding the start and ending LED from the clipboard diagram.
  for (int i = (rowNum * 8) - 8; i <= (rowNum * 8) - 1; i++) {
    leds[i] = CRGB(R, G, B);
  }
}
//---------------------------------------------------------------
void lightTile (int tileNum, int R, int G, int B) {              //lightTile lights the specified tile using the constant array which specifies which rows correspond to which tiles. This is a global variable.
  for (int i = 0; i <= 3; i++) {                              //Check the clipboard diagram for more details.
    lightRow(tileRow[tileNum - 1][i], R, G, B);
  }
}

void fadeTile (int tileNum, int R, int G, int B) {
  lightTile(tileNum, R, G, B);
  FastLED.show();
  for (int i = 255; i > 0; i = i - 20) {
    FastLED.setBrightness(i);
    FastLED.show();
  }
  lightTile(tileNum , 0, 0, 0);
  FastLED.show();
  FastLED.setBrightness(BRIGHTNESS);
}

void closeEncounters() {
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  safetyLight();
  FastLED.show();
  client.publish("/Egypt/Tomb/espAudio/SFX/Play/", "33");
  fadeTile(random(1, 32), 255, 0, 0);
  fadeTile(random(1, 32), 204, 54, 0);
  fadeTile(random(1, 32), 255, 135, 211);
  fadeTile(random(1, 32), 240, 240, 0);
  wait(250);
  fadeTile(random(1, 32), 255, 255, 255);
  wait(1250);
}

void fadeAll (int tileNum, int R, int G, int B, int incre) {
  fill_solid( leds, NUM_LEDS, CRGB(R, G, B));
  FastLED.show();
  for (int i = 200; i > 0; i = i - incre) {
    FastLED.setBrightness(i);
    FastLED.show();
  }
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  FastLED.show();
  FastLED.setBrightness(BRIGHTNESS);
}

void closeEncounters2() {
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  safetyLight();
  FastLED.show();
  client.publish("/Egypt/Tomb/espAudio/SFX/Play/", "34");
  fadeTile(random(1, 32), 255, 0, 0);
  fadeTile(random(1, 32), 204, 54, 0);
  fadeTile(random(1, 32), 255, 135, 211);
  fadeAll(random(1, 32), 240, 240, 0 , 20);
  wait(250);
  fadeAll(random(1, 32), 255, 255, 255, 12);
  wait(1000);
}
//---------------------------------------------------------------
int readTile(int threshold) {
  int tileInputs[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int sum = 0;                                                      // prints and returns the current Tile stepped on. WARNING: it cannot read multiple tiles at once. This is used primarily for
  int tilePressed = 0;                                              //simon but it can also be used for debugging as it will print to the serial monitor a tile is being pressed. The threshold parameter
  for (int i = startTile; i <= 15; i ++) {                        //is used to define what "pressed" is considered. A good standard value is 3000. Increasing the value will increase the tiles' sensitivity
    if (readMux(i, SIG_pin0) <= threshold) {                   //and vice versa. Do not go over a value of over 3800 as some tiles will trigger by the weight of the tile.
      tileInputs [i] = 1;
      tilePressed = i + 1;
      break;
    }
    else {
      tileInputs [i] = 0;
    }
  }

  for (int i = startTile; i <= 15; i++) {
    sum = sum + tileInputs[i];                                    //this checks to see if any tiles from the first mux are read. If there are the sum of its array will be 1. This also means if two tiles are
  }                                                               //read at the same time then the tile with the lower number will take prevalence. For example, if both tile 1 and 32 are depressed, the function
  //will return the integer 1 (and print "Tile 1") regardless of which tile was pressed first.
  for (int i = startTile; i <= 15; i ++) {
    if (readMux(i, SIG_pin1) <= threshold & sum != 1) {
      tileInputs [i + 16] = 1;
      tilePressed = i + 17;
      break;
    }
    else {
      tileInputs[i + 16] = 0;
    }
  }
  if (tilePressed != 0) {
    Serial.print("Tile ");
    Serial.println(tilePressed);
  }
  return tilePressed;
}
//---------------------------------------------------------------


//                                                                                                                    animations
//------------------------------------------------------------------------------------------------------------------------------
void safetyLight() {
  leds[832] = CRGB(25, 25, 25);
  leds[832 + 75] = CRGB(25, 25, 25);
  leds[832 + 150] = CRGB(25, 25, 25);
  leds[832 + 225] = CRGB(25, 25, 25);
  FastLED.show();

}
//------------------------------------------------------------------------------------------------------------------------------
void lightning() {
  int segmentSize = 25;
  for (int i = 832; i <= 906; i++) {
    leds[i] = CHSV(84, 0, 255);
    leds[i + 75] = CHSV(84, 0, 255);
    leds[i + 150] = CHSV(84, 0, 255);
    leds[i + 225] = CHSV(84, 0, 255);

    leds[i - segmentSize] = CHSV(84, 255, 0);
    leds[i + 75 - segmentSize] = CHSV(84, 255, 0);
    leds[i + 150 - segmentSize] = CHSV(84, 255, 0);
    leds[i + 225 - segmentSize] = CHSV(84, 255, 0);
    if (i % 12 == 0) {
      FastLED.show();
    }
    if (i == 906) {
      fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
      FastLED.show();
    }
  }
  outwardRay(255, 255, 255);
}
//------------------------------------------------------------------------------------------------------------------------------
void outwardRay(int R, int G, int B) {                            //Animation that starts from the center and radiates outward in all directions. Must specify color.
  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, R, G, B);
    }
    FastLED.show();
    wait(25);
  }

  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, 0, 0, 0);
    }
    FastLED.show();
    wait(35);
  }

}
//------------------------------------------------------------------------------------------------------------------------------
void rainbow_beat() {

  uint8_t beatA = beatsin8(17, 0, 255);                        // Starting hue
  uint8_t beatB = beatsin8(13, 0, 255);
  fill_rainbow(leds, NUM_LEDS, (beatA + beatB) / 2, 8);        // Use FastLED's fill_lightningbow routine.

}
//---------------------------------------------------------------
void grad() {
  for (int i = 0; i < 255; i++) {
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(0, 0, i), CRGB(i, 0, 0));
  }
}
//---------------------------------------------------------------
void blinkTile (int tileNum) {                                   //Blinks selected tile based on the tile number. Color is fixed, dim yellow. Generally used for testing purposes although can be used
  lightTile(tileNum, 100, 100, 0);                              //to quickly achieve the blinking effect on the tile.
  FastLED.show();
  wait(500);
  lightTile(tileNum, 0, 0, 0);
  FastLED.show();
  wait(500);
}
//---------------------------------------------------------------
void seqRow (int rowNum, int R, int G, int B) {                 //Lights a row starting from the center outwards. Must specify row number and color. Can only be used in sequentially iterative
  for (int i = 0; i <= 3; i++) {                               //loops (ie it must finish the animation before the next row can perform it0.)
    leds[((rowNum * 8) - 5) - i] = CRGB(R, G, B);
    leds[((rowNum * 8) - 4) + i] = CRGB(R, G, B);
    FastLED.show();
    FastLED.delay(12);
  }

}
//---------------------------------------------------------------
void circleTile (int tileNum, int R, int G, int B) {            //illuminates the specified tile using the seqRow animation to achieve an animated effect. Can often be used in place of lightTile but
  for (int i = 0; i <= 3; i++) {                               //will ignore any parallel lighting implementation, instead choosing to opt for a sequential illumination.
    seqRow(tileRow[tileNum - 1][i], R, G, B);
  }
}
//---------------------------------------------------------------
void spiralTile(int R, int G, int B) {                          //produces a spiral movement for the tiles from the center clockwise
  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, R, G, B);
      FastLED.show();
      FastLED.delay(7 - j);                                           //delay scales on the current ring, each outer ring will be 1 ms faster than the previous, which helps keep the flow of the animation smooth.
    }
  }

  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, 0, 0, 0);
      FastLED.show();
      FastLED.delay(7 - j);
    }
  }
}
//---------------------------------------------------------------
//                                                                                                                         modes
//------------------------------------------------------------------------------------------------------------------------------
void simonMode (int difficulty) {
  mqttSimonSolve = false;
  int tileSequence [20] = {};
  int tileInputs [20] = {};
  int currentTile = 0;
  int gameOver = 0;

  //Tile order generation
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  safetyLight();
  FastLED.show();
  for (int i = 0; i < 20; i++) {
    tileSequence[i] = random(1, 33);
    Serial.print(tileSequence[i]);
    Serial.print(" ");
  }
  Serial.println(" ");

  //**********GAME BEGINS*************//
  client.loop();
  checkConnection();
  client.publish("/Egypt/Tomb/", "Simon Puzzle Begin");
  for (int currentRound = 1; currentRound <= difficulty; currentRound++) {
    client.loop();
    if (mqttSimonSolve == true) {
      break;
    }
    Serial.print("Round ");
    Serial.print(currentRound);
    Serial.println(" ");
    if (gameOver == 0) {
      for (int j = 0; j < currentRound; j++) {                    //blink tile pattern

        if (currentRound != 1) {
          String tilenum = String(tileSequence[j]);
          client.publish("/Egypt/Tomb/espAudio/SFX/Play/", (char*) tilenum.c_str());
          blinkTile(tileSequence[j]);
        }
      }

      for (int j = 0; j < currentRound & gameOver == 0; j++) {    //accept tile input

        for (int i = 0; i <= 9; i++) {
          tileInputs[i] = 0;
        }
        currentTile = 0;
        int counter = 0;
        while (currentTile == 0) {                                //wait until input
          client.loop();
          checkConnection();
          if (mqttSimonSolve == true) {
            break;
          }
          currentTile = readTile(3200);
          if (currentRound == 1) {
            if (counter == 0) {
              String tilenum = String(tileSequence[0]);
              client.publish("/Egypt/Tomb/espAudio/SFX/Play/", (char*) tilenum.c_str());
              counter++;
            }
            lightTile(tileSequence[0], 100, 100, 0);              //keep first round light on until input
            FastLED.delay(250);
            FastLED.show();
          }

        }

        tileInputs[j] = currentTile;                                //populate current tile input into input array

        for (int i = 0; i <= 9; i++) {

          Serial.print(tileInputs[i]);
          Serial.print(" ");
        }
        Serial.println(" ");

        if (tileSequence[j] != tileInputs[j]) {                     //compare input array and pattern array
          gameOver = 1;
          String tilenum = String(tileInputs[j]);
          client.publish("/Egypt/Tomb/espAudio/SFX/Play/", "35");
          lightTile(currentTile, 255, 0, 0);
          FastLED.show();
          wait(500);
        }

        else if (tileSequence[j] == tileInputs[j]) {
          String tilenum = String(tileInputs[j]);
          client.publish("/Egypt/Tomb/espAudio/SFX/Play/", (char*) tilenum.c_str());
          lightTile(currentTile, 0, 255, 0);
          FastLED.show();
          wait(500);
          lightTile(currentTile, 0, 0, 0);
          FastLED.show();
          wait(500);
        }
      }
    }

    //**********GAME ENDS*************//
    Serial.println("Round End");

  }
  if (gameOver == 0 || mqttSimonSolve == true) {                                          //checks if the game is won or not
    closeEncounters2();
    simonSolved = true;
    checkConnection();
    client.publish("/Egypt/Tomb/", "Simon Puzzle Solved");
  }
  else if (gameOver == 1) {
    client.publish("/Egypt/Tomb/espAudio/SFX/", "35");
    Serial.println("game over");
    Serial.println("***************");
    outwardRay(255, 0, 0);
    outwardRay(255, 0, 0);
    simonSolved = false;
    checkConnection();
    client.publish("/Egypt/Tomb/", "Simon Puzzle Failed");
  }
  mqttSimonSolve = false;
}
//---------------------------------------------------------------
void demoMode (int R, int G, int B) {
  client.loop();
  checkConnection();                                          //demo mode allows for the floor to illuminate tiles as you stand on them. The parameters R G and B are for
  for (int i = startTile; i <= 15; i ++) {                       //the color of the tiles. The function populates an array with one bit for each tile. It also keeps a ledger
    if (readMux(i, SIG_pin0) <= 3000) {                      //that keeps track of the last state of the tile using an array of the same size. This way new instructions
      tileState [i] = 1;                                       //will only be relayed when the tile state changes instead of every cycle. This solves the flickering issue.
    }
    else {
      tileState [i] = 0;
    }
  }

  for (int i = startTile; i <= 15; i ++) {
    if (readMux(i, SIG_pin1) <= 3000) {
      tileState [i + 16] = 1;
    }
    else {
      tileState[i + 16] = 0;
    }
  }

  for (int i = startTile; i <= endTile; i ++) {
    if (tileState[i] == 1 & lastState[i] == 0) {
      for (int k = startTile; k <= endTile; k++) {
        if (i == k) {
          lightTile(i + 1, R, G, B);
          lastState[i] = 1;
          FastLED.show();
        }
      }
    }
    else if (tileState[i] == 0 & lastState[i] == 1) {          //These if statements check if adjacent tiles are currently lit and will recover lost color data for shared rows between tiles.
      lightTile(i + 1, 0, 0, 0);                               //Tiles can be adjacent if their tile numbers have an absolute difference of 1, 4, or 28. This phenomenon is due to the way in which
      lastState[i] = 0;                                        //the tiles are arranged and numbered.
      FastLED.show();
      if (tileState[i - 1] == 1) {
        lastState[i - 1] = 0;
      }
      if (tileState[i + 1] == 1) {
        lastState[i + 1] = 0;
      }
      if (tileState[i + 4] == 1) {
        lastState[i + 4] = 0;
      }
      if (tileState[i - 4] == 1) {
        lastState[i - 4] = 0;
      }
      if (tileState[i + 28] == 1) {
        lastState[i + 28] = 0;
      }
      if (tileState[i - 28] == 1) {
        lastState[i - 28] = 0;
      }
    }

  }
  for (int i = startTile; i <= endTile; i++) {                    //prints the two arrays, current tile state and last tile state. They will appear identical.
    Serial.print(tileState[i]);
  }
  Serial.print(" ");
  for (int i = startTile; i <= endTile; i++) {
    Serial.print(lastState[i]);
  }

  Serial.println("  ");
}
//---------------------------------------------------------------
void pressureDiagnoseMode() {                                     //A diagnosis mode that will show the tile and the pressure applied to that tile. Useful for debugging and
  //pressure tuning. 160 pounds of force should bring the value to between 150-300. Currently only works with mux 1.
  for (int i = 0; i <= 15; i ++) {                                //Sensor harness connectors will need to be rearranged in order to read tiles beyond the first 16.

    if (readMux(i, SIG_pin0) <= 3200) {
      Serial.print("Channel ");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(readMux(i, SIG_pin0));
      Serial.println(" ");
    }
  }
  for (int i = 0; i <= 15; i ++) {                                //Sensor harness connectors will need to be rearranged in order to read tiles beyond the first 16.

    if (readMux(i, SIG_pin1) <= 3200) {
      Serial.print("Channel ");
      Serial.print(i + 16);
      Serial.print(" ");
      Serial.print(readMux(i, SIG_pin1));
      Serial.println(" ");
    }
  }
}
//---------------------------------------------------------------
void StepPuzzle(int playerCount) {                                                             //The first puzzle of the room, this function contains a parameter corresponding to the number of
  int playerInputs [32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //guests in the experience. This number will light between 1-8 tiles, away from the door.
  int inputSum = 0;                                                                           //To solve this puzzle it will require that 1 person stands on each tile. Tile will change colors.
  int blinkState = 0;
  spSolved = false;
  mqttStepPuzzleSolve = false;
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  FastLED.show();
  safetyLight();
  checkConnection();
  client.publish("/Egypt/Tomb/", "Step Puzzle Started");

  while (spSolved == false) {
    for (int j = 0; j <= 119; j = j + 1) {
      if (j % (14 - playerCount) == 0) {
        blinkState = blinkState + 1;
      }
      if (spSolved == true) {
        break;
      }
      client.loop();
      for (int i = startTile; i <= 15; i ++) {
        client.loop();
        if (readMux(i, SIG_pin1) <= 2000) {
          playerInputs [i + 16] = 1;
        }
        else {
          playerInputs[i + 16] = 0;
        }
      }

      for (int i = 32; i >= 32 - (playerCount - 1) * 2; i = i - 2) {
        client.loop();
        if (playerInputs[i - 1] == 1) {
          lightTile(i, 0, 100, 0);
          FastLED.show();
        }
        else {
          if (blinkState % 2 == 0) {
            lightTile(i, 180, 25, 25);
          }
          else {
            lightTile(i, 0, 0, 0);
          }
          FastLED.show();
          playerInputs[i - 1] = 0;
        }
      }
      for (int i = 0; i <= 31; i++) {
        client.loop();
        inputSum = inputSum + playerInputs[i];
      }
      if (inputSum != playerCount) {
        inputSum = 0;
      }
      if (inputSum == playerCount || mqttStepPuzzleSolve == true) {
        spSolved = true;
        checkConnection();
        client.publish("/Egypt/Tomb/", "Step Puzzle Solved");
        outwardRay(0, 255, 0);
      }
    }
  }

  mqttStepPuzzleSolve = false;
}
//---------------------------------------------------------------


//                                                                                                            archived functions
//------------------------------------------------------------------------------------------------------------------------------

/*void pressureMeter (int sensorNum, int R, int G, int B) {
        if (readMux(sensorNum)<=150) {
     leds[0] = CRGB(255,0,0);
     leds[1] = CRGB(255,0,0);
     leds[2] = CRGB(255,0,0);
     leds[3] = CRGB(255,0,0);
     leds[4] = CRGB(255,0,0);
        }
       else if (readMux(sensorNum)<=450) {
     leds[0] = CRGB(0,255,0);
     leds[1] = CRGB(0,255,0);
     leds[2] = CRGB(0,255,0);
     leds[3] = CRGB(0,255,0);
     leds[4] = CRGB(0,255,0);
       }
       else if (readMux(sensorNum)<=500) {
     leds[0] = CRGB(R,G,B);
     leds[1] = CRGB(R,G,B);
     leds[2] = CRGB(R,G,B);
     leds[3] = CRGB(R,G,B);
       }
       else if (readMux(sensorNum)<=1000) {
     leds[0] = CRGB(R,G,B);
     leds[1] = CRGB(R,G,B);
     leds[2] = CRGB(R,G,B);
       }
       else if (readMux(sensorNum)<=2000) {
     leds[0] = CRGB(R,G,B);
     leds[1] = CRGB(R,G,B);
       }
       else if (readMux(sensorNum)<=3000) {
     leds[0] = CRGB(R,G,B);
       }
    if(readMux(sensorNum)>2500){
   leds[0] = CRGB::Black;
   leds[1] = CRGB::Black;
   leds[2] = CRGB::Black;
   leds[3] = CRGB::Black;
   leds[4] = CRGB::Black;

   FastLED.show();
   }

     }*/
//---------------------------------------------------------------
