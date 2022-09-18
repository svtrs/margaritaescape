#include <FastLED.h>
//Mux control pins
int s0 = 25;
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
#define NUM_LEDS    1140
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
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

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);


}


void loop() {
  simonMode(5);
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
//---------------------------------------------------------------
int readTile(int threshold) {
  int tileInputs[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int sum = 0;                                                      //readTile prints and returns the current Tile stepped on. WARNING: it cannot read multiple tiles at once. This is used primarily for
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
void outwardRay(int R, int G, int B) {                            //Animation that starts from the center and radiates outward in all directions. Must specify color.
  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, R, G, B);
    }
    FastLED.show();
    FastLED.delay(50);
  }

  for (int j = 1; j <= 4; j++) {
    for (int i = j; i <= 32; i = i + 4) {
      lightTile(i, 0, 0, 0);
    }
    FastLED.show();
    FastLED.delay(75);
  }

}
//------------------------------------------------------------------------------------------------------------------------------
void rainbow_beat() {

  uint8_t beatA = beatsin8(17, 0, 255);                        // Starting hue
  uint8_t beatB = beatsin8(13, 0, 255);
  fill_rainbow(leds, NUM_LEDS, (beatA + beatB) / 2, 8);        // Use FastLED's fill_rainbow routine.

}
//---------------------------------------------------------------
void grad() {
 for(int i = 0;i<255;i++){
 fill_gradient_RGB(leds,NUM_LEDS, CRGB(0,0,i), CRGB(i,0,0));
 }
}
//---------------------------------------------------------------
void blinkTile (int tileNum) {                                   //Blinks selected tile based on the tile number. Color is fixed, dim yellow. Generally used for testing purposes although can be used
  lightTile(tileNum, 100, 100, 0);                              //to quickly achieve the blinking effect on the tile.
  FastLED.show();
  FastLED.delay(500);
  lightTile(tileNum, 0, 0, 0);
  FastLED.show();
  FastLED.delay(250);
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
boolean simonMode (int difficulty) {
  //unsigned long timeElapsed = millis();
  int tileSequence [10] = {};
  int tileInputs [10] = {};
  int currentTile = 0;
  int gameOver = 0;
  boolean solved = false;

  //Tile order generation
  for (int i = 0; i < 10; i++) {
    tileSequence[i] = random(1, 33);
    Serial.print(tileSequence[i]);
    Serial.print(" ");
  }
  Serial.println(" ");

  //**********GAME BEGINS*************//

  for (int currentRound = 1; currentRound <= difficulty; currentRound++) {
    Serial.print("Round ");
    Serial.print(currentRound);
    Serial.println(" ");
    if (gameOver == 0) {

      for (int j = 0; j < currentRound; j++) {                    //blink tile pattern
        blinkTile(tileSequence[j]);
      }

      for (int j = 0; j < currentRound & gameOver == 0; j++) {    //accept tile input

        for (int i = 0; i <= 9; i++) {
          tileInputs[i] = 0;
        }
        currentTile = 0;
        while (currentTile == 0) {                                //wait until input
          currentTile = readTile(3000);
          if (currentRound == 1) {
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
          lightTile(currentTile, 255, 0, 0);
          FastLED.show();
          FastLED.delay(25);
        }

        else if (tileSequence[j] == tileInputs[j]) {
          lightTile(currentTile, 0, 255, 0);
          FastLED.show();
          FastLED.delay(500);
          lightTile(currentTile, 0, 0, 0);
          FastLED.show();
          FastLED.delay(250);
        }
      }
    }

    //**********GAME ENDS*************//
    Serial.println("Round End");

  }
  if (gameOver == 0) {                                            //checks if the game is won or not
    outwardRay(0, 255, 0);
    outwardRay(0, 255, 0);
    solved = true;
  }
  else if (gameOver == 1) {
    Serial.println("game over");
    Serial.println("***************");
    outwardRay(255, 0, 0);
    outwardRay(255, 0, 0);
    solved = false;
  }
  return solved;
}
//---------------------------------------------------------------
void demoMode (int R, int G, int B) {                           //demo mode allows for the floor to illuminate tiles as you stand on them. The parameters R G and B are for
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
void pressureDiagnoseMode()                                      //A diagnosis mode that will show the tile and the pressure applied to that tile. Useful for debugging and
{ //pressure tuning. 160 pounds of force should bring the value to between 150-300. Currently only works with mux 1.
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
void stepMode(int playerCount) {                                                             //The first puzzle of the room, this function contains a parameter corresponding to the number of
  int playerInputs [32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //guests in the experience. This number will light between 1-8 tiles, away from the door.
  int inputSum = 0;                                                                           //To solve this puzzle it will require that 1 person stands on each tile. Tile will change colors.
  boolean solved = false;

  for (int i = startTile; i <= 15; i ++) {
    if (readMux(i, SIG_pin1) <= 2000) {
      playerInputs [i + 16] = 1;
    }
    else {
      playerInputs[i + 16] = 0;
    }
  }

  for (int i = 32; i >= 32 - (playerCount - 1) * 2; i = i - 2) {
    if (playerInputs[i - 1] == 1) {
      lightTile(i, 0, 255, 0);
      FastLED.show();
    }
    else {
      lightTile(i, 255, 0, 0);
      FastLED.show();
    }
  }
  for (int i = 0; i <= 31; i++) {
    inputSum = inputSum + playerInputs[i];
  }
  Serial.println(inputSum);
  if (inputSum == playerCount) {
    solved = true;
    outwardRay(0, 255, 0);
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

     }*/
//---------------------------------------------------------------
