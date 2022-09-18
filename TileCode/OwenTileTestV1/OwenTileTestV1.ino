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
  //Current working version
  
    /*if (readMux(i)<=225){
    
   
    delay(5);
    if (i==0) {
      leds[0] = CRGB::Blue;
      }
    else if (i==1){
      leds[1] = CRGB(255,0,0);
      }
    else if (i==2) {
      leds[2] = CRGB(0,255,0);
      }
    else if (i==3){
      leds[3] = CRGB::Yellow;
      }
     
      FastLED.show();
    */
    for(int i = 0; i < 4; i ++){
      if (readMux(i)<=3800){
        delay(20);
    Serial.print("Value at channel ");
    Serial.print(i);
    Serial.print("is : ");
    Serial.println(readMux(i));
      }
      while (readMux(i)<=3000){
        








        
      if (i==0) {
     lightRow(1,115,115,115);
     lightRow(12,115,115,115);
     lightRow(9,115,115,115); 
      }
      else if (i==1) {
     lightRow(2,0,255,255);
     lightRow(11,0,255,255);
     lightRow(8,0,255,255);
     lightRow(12,0,255,255);
      }
      else if (i==2) {
     lightRow(3,0,0,255);
     lightRow(7,0,0,255);
     lightRow(10,0,0,255);
     lightRow(11,0,0,255);
      }
      else if (i==3) {
     lightRow(4,255,0,255);
     lightRow(5,255,0,255);
     lightRow(6,255,0,255);
     lightRow(10,255,0,255);
      }
      }
      fill_solid(leds, NUM_LEDS,CRGB(0,0,0));
      
      }
      
      
      FastLED.show();
  
    
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
  for(int i = 0; i < 4; i ++){
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
  FastLED.show();
  }
  
