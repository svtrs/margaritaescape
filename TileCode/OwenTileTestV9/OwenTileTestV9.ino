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

boolean decodeSolved = false;
boolean mqttDecodeSolve = false;

boolean mqttStepPuzzleSolve = false;
boolean spSolved = false;

boolean mqttDemoModeStart = false;
boolean mqttDemoModeEnd = false;

boolean pacifica = false;
boolean rainbow = false;

int playerCount = 2;
int simonDifficulty = 4;

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

void outwardRay(int R, int G, int B, boolean HSV = false);
void lightTile (int tileNum, int R, int G, int B, boolean HSV = false);
int readTile(int threshold, boolean skip = false);

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
    String clientId = "ESP32Tiles-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/Egypt/Tomb/", "Floor tiles online");
      // ... and resubscribe
      client.subscribe("/Egypt/Tomb/Tiles/");
      client.subscribe(mqttFullTopic("Simon/"));
      client.subscribe(mqttFullTopic("stepPuzzle/"));
      client.subscribe(mqttFullTopic("decodePuzzle/"));
      client.subscribe(mqttFullTopic("Animations/"));
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
    client.publish("/Egypt/Tomb/", "Simon Puzzle Begin");
    closeEncounters();
    while (simonSolved == false) {
      simon(simonDifficulty);
      wait(2000);
    }
    closeEncounters2();
    client.publish("/Egypt/Tomb/", "Simon Puzzle Solved");
    simonSolved = false;
  }
  else if (!strcmp(topic, mqttFullTopic("Simon/")) & message == "solve") {
    mqttSimonSolve = true;
  }
  else if (!strcmp(topic, mqttFullTopic("stepPuzzle/")) & message == "start") {
    client.publish("/Egypt/Tomb/", "Step Puzzle Started");
    StepPuzzle(playerCount);
    client.publish("/Egypt/Tomb/", "Step Puzzle Solved");
  }
  else if (!strcmp(topic, mqttFullTopic("stepPuzzle/")) & message == "solve") {
    mqttStepPuzzleSolve = true;
  }
  else if (!strcmp(topic, mqttFullTopic("decodePuzzle/")) & message == "start") {
    mqttDecodeSolve = false;
    client.publish("/Egypt/Tomb/", "Entering decode puzzle");
    spiralTile(0, 200, 200);
    while (!mqttDecodeSolve) {
      decodeSolved = false;
      while (decodeSolved == false) {
        decodeMode();
      }
      client.publish("/Egypt/Tomb/", "Array decoded");
      wait(2000);
    }

  }
  else if (!strcmp(topic, mqttFullTopic("decodePuzzle/")) & message == "solve") {
    mqttDecodeSolve = true;
  }
  else if (!strcmp(topic, mqttFullTopic("Animations/")) & message == "idle") {
    rstanimations();
    pacifica = true;
  }
  else if (!strcmp(topic, mqttFullTopic("Animations/")) & message == "rainbow") {
    rstanimations();
    rainbow = true;
  }
  else if (!strcmp(topic, mqttFullTopic("Animations/")) & message == "off") {
    rstanimations();
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
void rstanimations() {
  pacifica = false;
  rainbow = false;
}

void checkConnection() {
  if (!client.connected()) {
    Serial.println("Lost connection");
    reconnect();
  }
}


void loop() {
  client.loop();
  checkConnection();

  while (pacifica) {
    client.loop();
    checkConnection();
    EVERY_N_MILLISECONDS(10) {
      pacifica_loop();
    }
    FastLED.show();
  }

  while (rainbow) {
    client.loop();
    checkConnection();
    rainbow_beat();
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
void lightRow (int rowNum, int R, int G, int B, boolean HSV = false) {                //lightRow lights the specified row by finding the start and ending LED from the clipboard diagram.
  for (int i = (rowNum * 8) - 8; i <= (rowNum * 8) - 1; i++) {
    if (!HSV) {
      leds[i] = CRGB(R, G, B);
    }
    else {
      leds[i] = CHSV(R, G, B);
    }
  }
}
//---------------------------------------------------------------
void lightTile (int tileNum, int R, int G, int B, boolean HSV) {              //lightTile lights the specified tile using the constant array which specifies which rows correspond to which tiles. This is a global variable.

  for (int i = 0; i <= 3; i++) {                              //Check the clipboard diagram for more details.
    lightRow(tileRow[tileNum - 1][i], R, G, B, HSV);
  }
}


void lightDiagonal (int diagonal, int V = 255) {
  int H, firstTile;
  switch (diagonal) {
    case 1:
      firstTile = 5;
      H = 0;
      break;
    case 2:
      firstTile = 13;
      H = 160;
      break;
    case 3:
      firstTile = 21;
      H = 64;
      break;
    case 4:
      firstTile = 29;
      H = 96;
      break;
  }
  for (int i = firstTile; i < firstTile + 4; i++) {
    lightTile(i, H, 255, V, true);
  }
  if (V != 0) {
    String tilenum = String(firstTile);
    client.publish("/Egypt/Tomb/espAudio/SFX/Play/", (char*) tilenum.c_str());
  }
}

void blinkDiagonal(int diagonal) {
  lightDiagonal(diagonal);
  FastLED.show();
  wait(500);
  lightDiagonal(diagonal, 0);
  FastLED.show();
  wait(500);
}


void fadeTile (int tileNum, int R, int G, int B) {
  lightTile(tileNum, R, G, B, false);
  FastLED.show();
  for (int i = 200; i > 0; i = i - 20) {
    FastLED.setBrightness(i);
    FastLED.show();
  }
  lightTile(tileNum , 0, 0, 0, false);
  FastLED.show();
  FastLED.setBrightness(BRIGHTNESS);
}

void fadeObelisk(int H, int S) {
  lightObelisk(H, S, 200);
  FastLED.show();
  for (int i = 200; i > 0; i = i - 20) {
    FastLED.setBrightness(i);
    FastLED.show();
  }
  lightObelisk(H, S, 0);
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

void fadeAll (int R, int G, int B, int incre) {
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
  fadeAll(240, 240, 0 , 20);
  wait(250);
  fadeAll(255, 255, 255, 12);
  wait(1000);
}
//---------------------------------------------------------------
int readTile(int threshold, boolean skip) {
  int tileInputs[32] = {0};
  int sum = 0;                                                      // prints and returns the current Tile stepped on. WARNING: it cannot read multiple tiles at once. This is used primarily for
  int tilePressed = 0;                                              //simon but it can also be used for debugging as it will print to the serial monitor a tile is being pressed. The threshold parameter
  boolean dontRead = false;                                         //is used to define what "pressed" is considered. A good standard value is 3000. Increasing the value will increase the tiles' sensitivity
  for (int i = startTile; i <= 15; i++) {
    int j = i + 1;
    if (skip) {
      switch (j) {
        case 1: case 2: case 3: case 4: case 9: case 10: case 11: case 12:
          dontRead = true;
          break;
        default:
          dontRead = false;
          break;
      }
    }
    if (dontRead) {
      continue;
    }
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
  dontRead = false;                                                                //will return the integer 1 (and print "Tile 1") regardless of which tile was pressed first.

  for (int i = startTile; i <= 15; i ++) {
    int j = i + 17;
    if (skip) {
      switch (j) {
        case 17: case 18: case 19: case 20: case 25: case 26: case 27: case 28:
          dontRead = true;
          break;
      }
    }

    if (dontRead) {
      dontRead = false;
      continue;
    }

    if (readMux(i, SIG_pin1) <= threshold && sum != 1) {
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
void lightning(int H = 0, int S = 0) {
  int segmentSize = 25;
  for (int i = 832; i <= 906; i++) {
    leds[i] = CHSV(H, S, 255);
    leds[i + 75] = CHSV(H, S, 255);
    leds[i + 150] = CHSV(H, S, 255);
    leds[i + 225] = CHSV(H, S, 255);

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
  outwardRay(H, S, 255, true);
}
//------------------------------------------------------------------------------------------------------------------------------
void lightColumn(int column, int lednum, int H, int S, int V) {
  //906-832
  int firstLed;
  int lastLed;
  firstLed = 906 + (75 * (column - 1));
  lastLed = firstLed - lednum + 1;
  leds[lastLed] = CHSV(H, S, V);
}

void lightObelisk(int H, int S, int V) {
  for (int i = 1; i < 76; i++) {
    lightColumn(1, i, H, S, V);
    lightColumn(2, i, H, S, V);
    lightColumn(3, i, H, S, V);
    lightColumn(4, i, H, S, V);
  }
}
void fail(int lightspeed) {
  for (int i = 1; i < 32; i = i + 4) {
    lightTile(i, 64, 255, 255, true);
  }
  for (int i = 1; i <= 54; i++) {
    lightColumn(1, i, 64, 255, 255);
    lightColumn(2, i, 64, 255, 255);
    lightColumn(3, i, 64, 255, 255);
    lightColumn(4, i, 64, 255, 255);
    if (!(i % lightspeed)) {
      FastLED.show();
    }
    wait(i * 2);
  }

  for (int i = 54; i > 1; i--) {
    lightColumn(1, i, 64, 255, 0);
    lightColumn(2, i, 64, 255, 0);
    lightColumn(3, i, 64, 255, 0);
    lightColumn(4, i, 64, 255, 0);
    if (!(i % lightspeed)) {
      FastLED.show();
    }
  }
}
//------------------------------------------------------------------------------------------------------------------------------
void outwardRay(int R, int G, int B, boolean HSV) {                            //Animation that starts from the center and radiates outward in all directions. Must specify color.
  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, R, G, B, HSV);
    }
    FastLED.show();
    wait(25);
  }

  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, 0, 0, 0, HSV);
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
void blinkTile (int tileNum) {                                   //Blinks selected tile based on the tile number. Color is fixed, dim yellow. Generally used for testing purposes although can be used
  lightTile(tileNum, 100, 100, 0, false);                             //to quickly achieve the blinking effect on the tile.
  FastLED.show();
  wait(500);
  lightTile(tileNum, 0, 0, 0, false);
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
      lightTile(i, R, G, B, false);
      FastLED.show();
      FastLED.delay(7 - j);                                           //delay scales on the current ring, each outer ring will be 1 ms faster than the previous, which helps keep the flow of the animation smooth.
    }
  }

  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, 0, 0, 0, false);
      FastLED.show();
      FastLED.delay(7 - j);
    }
  }
}
//---------------------------------------------------------------
//                                                                                                                         modes
//------------------------------------------------------------------------------------------------------------------------------
void decodeMode() {
  //99 is my chosen null value.
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  FastLED.show();
  decodeSolved = false;
  int correctArray[8][6] =
  {
    //obelisk top
    {22, 18, 0, 0, 0, 0},   //ra
    {31, 18, 7, 19, 0, 0},  //bask
    {18, 3, 6, 31, 27, 7},  //anubis
    {23, 11, 22, 6, 7, 0},  //horus

    //relics
    {10, 15, 6, 0, 0, 0},   //ra (relic)
    {30, 27, 7, 0, 0, 0},   //bask (relic)
    {2, 31, 14, 0, 0, 0},   //anubis (relic)
    {26, 14, 30, 0, 0, 0}   //horus (relic)
  };
  int arrayRowIndex = 10;
  int activeTiles[16] = {2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31};
  int inputArray[6] = {};
  int temp = 0;
  boolean wloop = true;

  //begin process
  for (int j = 0; j < 6; j++) {
    Serial.print("starting iteration ");
    Serial.print(j);
    Serial.println(" ");

    if (correctArray[arrayRowIndex][j] == 0) {
      break;
    }

    //take input
    while (wloop) {
      client.loop();
      if(mqttDecodeSolve){
        decodeSolved = true;
        return;
      }
      temp = readTile(3200);
      //filter out inactive tiles
      for (int i = 0; i < 16; i++) {
        if (temp == activeTiles[i]) {
          wloop = false;
          break;
        }
      }
    }//while(!temp)
    fadeTile(temp, 0, 255, 255);
    wloop = true;

    wait(1000);
    if (arrayRowIndex == 10) {
      switch (temp) {
        case 22://correctArray[0][0]
          arrayRowIndex = 0;
          break;
        case 31://correctArray[1][0]
          arrayRowIndex = 1;
          break;
        case 18://correctArray[2][0]
          arrayRowIndex = 2;
          break;
        case 23://correctArray[3][0]
          arrayRowIndex = 3;
          break;
        case 10://correctArray[4][0]
          arrayRowIndex = 4;
          break;
        case 30://correctArray[5][0]
          arrayRowIndex = 5;
          break;
        case 2://correctArray[6][0]
          arrayRowIndex = 6;
          break;
        case 26://correctArray[7][0]
          arrayRowIndex = 7;
          break;
        default:
          arrayRowIndex = 99;
          break;
      }
    }
    inputArray[j] = temp;

    if (arrayRowIndex == 99 || inputArray[j] != correctArray[arrayRowIndex][j]) {
      for (int i = 5; i < 9; i++) {
        lightTile(i, 255, 0, 0);
        lightTile(i + 8, 255, 0, 0);
        lightTile(i + 16, 255, 0, 0);
        lightTile(i + 24, 255, 0, 0);
      }
      client.publish("/Egypt/Tomb/espAudio/SFX/Play/", "35");
      FastLED.show();
      decodeSolved = false;
      wait(500);
      for (int i = 5; i < 9; i++) {
        lightTile(i, 0, 0, 0);
        lightTile(i + 8, 0, 0, 0);
        lightTile(i + 16, 0, 0, 0);
        lightTile(i + 24, 0, 0, 0);
      }
      FastLED.show();
      return;
    }

  }

  wait(2000);
  switch (arrayRowIndex) {
    case 0://ra
      fadeObelisk(0, 0);
      fadeObelisk(0, 0);
      break;
    case 1://bask
      client.publish("/Egypt/Tomb/", "bask");
      for (int i = 0; i < 3; i++) {
        fadeTile(32, 255, 0, 0);
        wait(250);
      }
      break;
    case 2://anubis
      client.publish("/Egypt/Tomb/", "anubis");
      for (int i = 0; i < 3; i++) {
        fadeTile(4, 0, 255, 0);
        wait(250);
      }
      break;
    case 3://horus
      client.publish("/Egypt/Tomb/", "horus");
      for (int i = 0; i < 3; i++) {
        fadeTile(8, 0, 0, 255);
        wait(250);
      }
      break;
    case 4://ra relic
      fadeAll(200, 200, 200, 12);
      wait(250);
      fadeAll(200, 200, 200, 12);
      break;
    case 5://bask relic
      fadeAll(200, 0, 0, 12);
      wait(250);
      fadeAll(200, 0, 0, 12);
      break;
    case 6://abubis relic
      fadeAll(0, 200, 0, 12);
      wait(250);
      fadeAll(0, 200, 0, 12);
      break;
    case 7://horus relic
      fadeAll(0, 0, 200, 12);
      wait(250);
      fadeAll(0, 0, 200, 12);
      break;
  }
  decodeSolved  = true;
}
//------------------------------------------------------------------------------------------------------------------------------
void simon(int rounds) {
  mqttSimonSolve = false;
  const int offTiles[16] = {1, 2, 3, 4, 9, 10, 11, 12, 17, 18, 19, 20, 25, 26, 27, 28};
  int tileSequence [20] = {};
  int tileInputs [20] = {};
  int currentTile = 0;

  //Tile order generation
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 0));
  safetyLight();
  FastLED.show();
  for (int i = 0; i < 20; i++) {
    tileSequence[i] = random(1, 5);
    Serial.print(tileSequence[i]);
    Serial.print(" ");
  }
  client.loop();
  checkConnection();
  for (int currentRound = 1; currentRound <= rounds; currentRound++) {
    client.loop();
    wait(150);
    if (mqttSimonSolve == true) {
      simonSolved = true;
      return;
    }
    Serial.println(" ");
    Serial.print("Round ");
    Serial.print(currentRound);
    Serial.println(" ");
    for (int j = 0; j < currentRound; j++) {                    //blink tile pattern
      blinkDiagonal(tileSequence[j]);
    }

    for (int j = 0; j < currentRound; j++) {    //accept tile input

      for (int i = 0; i < 20; i++) {
        tileInputs[i] = 0;
      }
      currentTile = 0;
      while (currentTile == 0) {                                //wait until input
        client.loop();
        checkConnection();
        if (mqttSimonSolve == true) {
          simonSolved = true;
          return;
        }
        currentTile = readTile(3300, true);                           //ignore tiles specified in array
        for (int i = 0; i < 16; i++) {
          if (currentTile == offTiles[i]) {
            currentTile = 0;
          }
        }
      }
      switch (currentTile) {                                    //Switch statement converting currentTile range to a diagonal value (1-4) clockwise when facing tomb exit.
        case 5:
        case 6:
        case 7:
        case 8:
          currentTile = 1;
          break;
        case 13:
        case 14:
        case 15:
        case 16:
          currentTile = 2;
          break;
        case 21:
        case 22:
        case 23:
        case 24:
          currentTile = 3;
          break;
        case 29:
        case 30:
        case 31:
        case 32:
          currentTile = 4;
          break;
      }

      tileInputs[j] = currentTile;                                //populate current tile input into input array

      for (int i = 0; i < 20; i++) {
        Serial.print(tileInputs[i]);
        Serial.print(" ");
      }
      Serial.println(" ");

      if (tileSequence[j] != tileInputs[j]) {                     //Puzzle loss state. If it makes it past this point, the game must have been won.
        client.publish("/Egypt/Tomb/espAudio/SFX/Play/", "35");
        client.publish("/Egypt/Tomb/", "Simon Puzzle Failed");
        lightning(0, 255);
        simonSolved = false;
        return;
      }

      else if (tileSequence[j] == tileInputs[j]) {
        blinkDiagonal(tileInputs[j]);
      }
    }
    Serial.println("Round End");
  }
  simonSolved = true;
  mqttSimonSolve = false;
  //**********GAME ENDS*************//
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
          lightTile(i + 1, R, G, B, false);
          lastState[i] = 1;
          FastLED.show();
        }
      }
    }
    else if (tileState[i] == 0 & lastState[i] == 1) {          //These if statements check if adjacent tiles are currently lit and will recover lost color data for shared rows between tiles.
      lightTile(i + 1, 0, 0, 0, false);                              //Tiles can be adjacent if their tile numbers have an absolute difference of 1, 4, or 28. This phenomenon is due to the way in which
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
  //guests in the experience. This number will light between 1-8 tiles, away from the door.
  int playerInputs [32] = {0};
  int inputSum = 0;
  int inactiveTiles[32] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
  for (int i = 31; i > 32 - (playerCount * 2); i = i - 2) {
    inactiveTiles[i] = 1;
  }
  spSolved = false;
  mqttStepPuzzleSolve = false;
  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
  safetyLight();
  FastLED.show();
  while (spSolved == false) {
    checkConnection();
    client.loop();
    if (mqttStepPuzzleSolve == true) {
      mqttStepPuzzleSolve = false;
      spSolved = true;
      return;
    }

    for (int i = startTile; i < 16; i ++) {
      if (readMux(i, SIG_pin1) <= 2000) {
        playerInputs [i + 16] = 1;
      }
      else {
        playerInputs[i + 16] = 0;
      }
    }

    for (int i = 32; i >= 32 - (playerCount - 1) * 2; i = i - 2) {
      lightObelisk(128, 255, beatsin8(30, 100, 200));
      if (playerInputs[i - 1] == 1) {
        lightTile(i, 96, 255, 175, true);
      }
      else {
        lightTile(i, 128, 255, beatsin8(30, 100, 200), true);
        /*
          pacifica_loop();
          for (int j = 0; j < 32; j++) {
          lightTile(inactiveTiles[j], 0, 0, 0);
          }*/
        playerInputs[i - 1] = 0;
      }
      FastLED.show();
    }

    for (int i = 0; i <= 31; i++) {
      inputSum = inputSum + playerInputs[i];
    }
    if (inputSum != playerCount) {
      inputSum = 0;
    }
    if (inputSum == playerCount) {
      spSolved = true;
      checkConnection();
      lightning(96, 255);
    }
  }
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

     }

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
    tileSequence[i] = random(1, 4);
    Serial.print(tileSequence[i]);
    Serial.print(" ");
  }
  Serial.println(" ");

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
            lightTile(tileSequence[0], 100, 100, 0, false);             //keep first round light on until input
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
          lightTile(currentTile, 255, 0, 0, false);
          FastLED.show();
          wait(500);
        }

        else if (tileSequence[j] == tileInputs[j]) {
          String tilenum = String(tileInputs[j]);
          client.publish("/Egypt/Tomb/espAudio/SFX/Play/", (char*) tilenum.c_str());
          lightTile(currentTile, 0, 255, 0, false);
          FastLED.show();
          wait(500);
          lightTile(currentTile, 0, 0, 0, false);
          FastLED.show();
          wait(500);
        }
      }
    }
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
  }
  mqttSimonSolve = false;
  }

*/
//---------------------------------------------------------------
