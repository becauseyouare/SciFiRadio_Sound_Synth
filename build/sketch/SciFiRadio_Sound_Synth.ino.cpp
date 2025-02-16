#line 1 "C:\\Users\\atomi\\Dropbox\\MyProjects2022-2025\\SciFiRadio_Sound_Synth\\SciFiRadio_Sound_Synth.ino"
#include <Arduino.h>
/* 
  using Mozzi sonification library.

  The circuit:
     Audio output on digital pin 9 (on a Uno or similar), or 
     check the README or http://sensorium.github.com/Mozzi/

  Tim Barrass 2013.
  This example code is in the public domain.
*/

#include <MozziGuts.h>
#include <Oscil.h> // oscillator 
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <Smooth.h>
#include <AutoMap.h> // maps unpredictable inputs to a range
 
// int freqVal;
const int carrFreq_M[] = {22,330,30,800,200,20,200,100,440,20,30,80,20,100,400,300,20,80,90,400,20};  // 22 to 880
const int ldr1_M[] = {70,300,100,50,400,100,80,700,30,60,10,80,200,300,100,40,10,60,90,200,70}; // 10 to 700
const int modSp_M[] = {1,50,9000,600,10,90,500,1000,2000,500,800,30,90,100,200,700,30,80,100,300,10}; // float (1 to 10000)/1000
const int frq_M[] = {10,4,8,10,6,4,1,1,6,6,7,7,3,2,1,2,10,4,7,5,8,9,10,10};
const int knob2_M[] = {0,1,3,2,7,6,9,3,4,5,8,3,2,2,1,2,6,7,8,3,9};
 
// desired carrier frequency max and min, for AutoMap
const int MIN_CARRIER_FREQ = 22;
const int MAX_CARRIER_FREQ = 880;

const int MIN = 1;
const int MAX = 10;

const int MIN_2 = 1;
const int MAX_2 = 15;

// desired intensity max and min, for AutoMap, note they're inverted for reverse dynamics
const int MIN_INTENSITY = 700;
const int MAX_INTENSITY = 10;

// desired mod speed max and min, for AutoMap, note they're inverted for reverse dynamics
const int MIN_MOD_SPEED = 10000;
const int MAX_MOD_SPEED = 1;

AutoMap kMapCarrierFreq(0,1023,MIN_CARRIER_FREQ,MAX_CARRIER_FREQ);
AutoMap kMapIntensity(0,1023,MIN_INTENSITY,MAX_INTENSITY);
AutoMap kMapModSpeed(0,1023,MIN_MOD_SPEED,MAX_MOD_SPEED);
AutoMap mapThis(0,1023,MIN,MAX);
AutoMap mapThisToo(0,1023,MIN_2,MAX_2);

const int KNOB_PIN = 0; // set the input for the knob to analog pin 0
const int LDR1_PIN=1; // set the analog input for fm_intensity to pin 1
const int LDR2_PIN=2; // set the analog input for mod rate to pin 2
const int LDR3_PIN=4; // set the analog input for mod rate to pin 2
const int LDR4_PIN=3; // set the analog input for mod rate to pin 2

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);

int dialPosition = 0;
int mod_ratio = 5; // brightness (harmonics)
long fm_intensity; // carries control info from updateControl to updateAudio

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);


#line 69 "C:\\Users\\atomi\\Dropbox\\MyProjects2022-2025\\SciFiRadio_Sound_Synth\\SciFiRadio_Sound_Synth.ino"
void setup();
#line 79 "C:\\Users\\atomi\\Dropbox\\MyProjects2022-2025\\SciFiRadio_Sound_Synth\\SciFiRadio_Sound_Synth.ino"
void updateControl();
#line 180 "C:\\Users\\atomi\\Dropbox\\MyProjects2022-2025\\SciFiRadio_Sound_Synth\\SciFiRadio_Sound_Synth.ino"
int updateAudio();
#line 186 "C:\\Users\\atomi\\Dropbox\\MyProjects2022-2025\\SciFiRadio_Sound_Synth\\SciFiRadio_Sound_Synth.ino"
void loop();
#line 69 "C:\\Users\\atomi\\Dropbox\\MyProjects2022-2025\\SciFiRadio_Sound_Synth\\SciFiRadio_Sound_Synth.ino"
void setup(){
  Serial.begin(115200); // set up the Serial output so we can look at the light level
  Serial.println("Hello Mozzi 21425");
  startMozzi(); // :))
  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
}


void updateControl(){

  int DebugMode = 0;
  if(digitalRead(2)) DebugMode = 1;

  // if(!digitalRead(3)){
  //   carrier_freq = 92;
  //   LDR1_calibrated = 152;
  //   mod_speed = 6.84;
  //   kIntensityMod.setFreq(mod_speed);
  //   FRQ = 1;
  //   knob2Val = 2;
  // }

  int pot0 = mozziAnalogRead(A3);
  int pot1 = mozziAnalogRead(A4);
  int potDif = pot0 - pot1;
  
  //dialPosition = mozziAnalogRead(KNOB_PIN); // value is 0-1023  // temporary input
  if((potDif > 6)||(potDif < -6)){
    dialPosition = dialPosition + potDif/4;
    if(dialPosition > 10230){
      dialPosition = 1;
    }else if(dialPosition < 1){
      dialPosition = 10230;
    }
    //Serial.println(dialPosition,DEC);
  }

   int a = dialPosition/500;
   int x = (dialPosition/10) %50;
  int carrier_freq = map(x,0,50,carrFreq_M[a],carrFreq_M[a+1]);
  int LDR1_value =  map(x,0,50,ldr1_M[a],ldr1_M[a+1]);
  float mod_speed =  map(x,0,50,modSp_M[a],modSp_M[a+1])/1000;
  int FRQ =  map(x,0,50,frq_M[a],frq_M[a+1]);
  int knob2Val =  map(x,0,50,knob2_M[a],knob2_M[a+1]);

//  freqVal = map(LDR3_PIN, 0, 1023, 1, 100);
  
  // read the knob
   //int knob_value = mozziAnalogRead(KNOB_PIN); // value is 0-1023
  // map the knob to carrier frequency
  // int carrier_freq = kMapCarrierFreq(knob_value);  //  ------Mapped-----

  // read the light dependent resistor on the width Analog input pin
  // int LDR1_value= mozziAnalogRead(LDR1_PIN); // value is 0-1023  -----Mapped-----
  int LDR1_calibrated = kMapIntensity(LDR1_value);

  // int LDR2_value= mozziAnalogRead(LDR2_PIN); // value is 0-1023
  // use a float here for low frequencies
  // float mod_speed = (float)kMapModSpeed(LDR2_value)/1000;   //  ----Mapped-----
  kIntensityMod.setFreq(mod_speed);

  // int freqVal = mozziAnalogRead(LDR3_PIN); // value is 0-1023
  // int FRQ = mapThis(freqVal);
  
  // int knob2 = mozziAnalogRead(LDR4_PIN); // value is 0-1023
  // int knob2Val = mapThis(knob2);

  //calculate the modulation frequency to stay in ratio
  int mod_freq = carrier_freq * mod_ratio * FRQ;
  
  // set the FM oscillator frequencies
  aCarrier.setFreq(carrier_freq); 
  aModulator.setFreq(mod_freq);
 // calculate the fm_intensity
  fm_intensity = ((long)LDR1_calibrated * knob2Val * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply


  // print the value to the Serial monitor for debugging
    if(DebugMode){
    Serial.print("D = "); 
    Serial.print(dialPosition);
    Serial.print(" "); // prints a tab
    Serial.print("a = "); 
    Serial.print(a);
    Serial.print(" "); // prints a tab
    Serial.print("x = "); 
    Serial.print(x);
    Serial.print("\t"); // prints a tab
    Serial.print("carrier_freq = "); 
    Serial.print(carrier_freq);
    Serial.print("\t"); // prints a tab
    Serial.print("LDR1_calibrated = "); 
    Serial.print(LDR1_calibrated);
    Serial.print("\t"); // prints a tab
    Serial.print("mod_speed = ");
    Serial.print(mod_speed);
    Serial.print("\t"); // prints a tab
    Serial.print("  FRQ = "); 
    Serial.print(FRQ);
    Serial.print("\t"); // prints a tab
    Serial.print("knob2val = "); 
    Serial.print(knob2Val);
    Serial.println(); // finally, print a carraige return for the next line of debugging info
  }

  
}


int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  return aCarrier.phMod(modulation);
}


void loop(){
  audioHook();
}
