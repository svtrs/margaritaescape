#include <FastLED.h>
//Mux control pins
int s0 = 25;
int s1 = 23;
int s2 = 22;
int s3 = 21;

//Mux in "SIG" pin
int SIG_pin = 32;

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    96
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120
int tileState [12] = {0,0,0,0,0,0,0,0,0,0,0,0};   //specify number of tiles in tileState[] and lastState
int lastState [12] = {0,0,0,0,0,0,0,0,0,0,0,0};
const int nullRow = 105;
const int tileRow [16][4]={       //tileRow[number of tiles][number of rows]
//105 is a null row for tiles with 3 sides

{12,9,nullRow,1},   //Tile 1
{11,8,12,2},    //Tile 2                        
{10,7,11,3},    //Tile 3
{5,6,10,4},     //Tile 4

{13,26,27,9},   //Tile 5
{15,24,25,14},  //Tile 6
{17,22,23,16},  //Tile 7
{19,20,21,18},  //Tile 8

{27,38,35,nullRow}, //Tile 9
{28,37,34,38},  //Tile 10
{29,36,33,37},  //Tile 11
{30,31,32,36},  //Tile 12

{35,39,52,53},  //Tile 13
{40,41,50,51},  //Tile 14
{42,43,48,49},  //Tile 15
{44,45,46,47},  //Tile 16

};
 

void setup(){
  pinMode(s0, OUTPUT); 
  pinMode(s1, OUTPUT); 
  pinMode(s2, OUTPUT); 
  pinMode(s3, OUTPUT); 

  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);

  Serial.begin(115200);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  
}


void loop(){
  //Loop through and read all 16 values
  //Reports back Value at channel 6 is: 346
    
    
    for(int i = 0; i <= 11; i ++){
      
        if (readMux(i)<=3000){
        tileState [i]=1;
          }
        else {
        tileState [i]=0;
        }
      }

   for(int i = 0; i <= 11; i ++){
      if (tileState[i]==1 & lastState[i]==0){
        if (i==0) {
        lightTile(i+1,255,255,255); 
        lastState[i] = 1;
        FastLED.show();
        }

        else if (i==1){
        lightTile(i+1,255,255,0); 
        lastState[i] = 1;
        FastLED.show();
        }

        else if (i==2){
        lightTile(i+1,0,25,255); 
        lastState[i] = 1;
        FastLED.show();
        }

        else if (i==3){
        lightTile(i+1,255,0,255); 
        lastState[i] = 1;
        FastLED.show();
        }

        if (i==8) {
        lightTile(i+1,255,255,255); 
        lastState[i] = 1;
        FastLED.show();
        }

        else if (i==9){
        lightTile(i+1,255,255,0); 
        lastState[i] = 1;
        FastLED.show();
        }

        else if (i==10){
        lightTile(i+1,0,25,255); 
        lastState[i] = 1;
        FastLED.show();
        }

        else if (i==11){
        lightTile(i+1,255,0,255); 
        lastState[i] = 1;
        FastLED.show();
        }
        
   }
      else if (tileState[i]==0 & lastState[i]==1) {
        lightTile(i+1,0,0,0);
        lastState[i] = 0;
        FastLED.show();
          if(tileState[i-1]==1){
            lastState[i-1] = 0;
          }
          if (tileState[i+1]==1){
              lastState[i+1] = 0;
          }
        }
      
   }

    for (int i=0; i<=11; i++){
    Serial.print(tileState[i]);
    }
    Serial.print(" ");
    for (int i=0; i<=11; i++){
    Serial.print(lastState[i]);
    }
    
    Serial.println("  ");
    
  }

  
int readMux(int channel){
  int controlPin[] = {s0, s1, s2, s3};

  int muxChannel[16][4]={
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
  };

  //loop through the 4 sig
  for(int i = 0; i <= 3; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  //read the value at the SIG pin
  int val = analogRead(SIG_pin);

  //return the value
  return val;
}

void pressureMeter (int sensorNum, int R, int G, int B) {
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

  void lightRow (int rowNum, int R, int G, int B) {
for (int i=(rowNum*8)-8; i<=(rowNum*8)-1; i++){
  leds[i] = CRGB(R,G,B);
  }
  }

  void lightTile (int tileNum, int R, int G, int B) {
      for(int i = 0; i <= 3; i++){
    lightRow(tileRow[tileNum-1][i],R,G,B);
      }
    }

  void seqRow (int rowNum, int R, int G, int B) {
for (int i=(rowNum*8)-8; i<=(rowNum*8)-1; i++){
  leds[i] = CRGB(R,G,B);
  FastLED.show();
  delay(7);
  }
  }

  void circleTile (int tileNum, int R, int G, int B) {
      for(int i = 0; i <= 3; i++){
    seqRow(tileRow[tileNum-1][i],R,G,B);
    delay(56);
      }
    }
  
  void blinkTile (int tileNum){
    lightTile(tileNum,255,255,255);
    delay(500);
    lightTile(tileNum,0,0,0);
    delay(500);
    }

 void ants (){
  for(int i = 1; i<=424;i++){
      int R = random(255), G = random(255), B = random(255);
      leds[i] = CRGB(R,G,B);
      delay (25);
      Serial.println("Tile on");
      leds[i-2] = CRGB(0,0,0);
      FastLED.show();
    }
 }
  
  
