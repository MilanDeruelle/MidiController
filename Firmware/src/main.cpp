/*
 Midi Controller Code
 Created by Milan Deruelle
 08 April 2020
 */

#include <Arduino.h>
#include <MIDIUSB.h>



#define AVERAGE_SIZE 4
#define NUM_POTIS_PER_BOARD 8
#define NUM_BOARDS 3
#define NUM_MULTIPLEXER_PINS 3
#define CHANNEL 1


byte values[NUM_POTIS_PER_BOARD*NUM_BOARDS];
int  runningAverageValues[NUM_POTIS_PER_BOARD*NUM_BOARDS*AVERAGE_SIZE];
int  runningAverage = 0;

static const byte CC_NUMBER[] = {3,9,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,85,86,87,89,90,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119};
static const uint8_t MULTIPLEXER_PINS[] = {14,16,10};
static const uint8_t INPUT_PINS[] = {A2,A1,A0};




void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN_TX,INPUT);
  pinMode(LED_BUILTIN_RX,INPUT);
  
  for(int i = 0; i<NUM_MULTIPLEXER_PINS; i++){
    pinMode(MULTIPLEXER_PINS[i], OUTPUT);
  }
  for(int i = 0; i<NUM_BOARDS; i++){
    pinMode(INPUT_PINS[i], INPUT);
  }

}
byte potiIdentifier(byte board, byte poti, int multiplier){
  return NUM_BOARDS*NUM_POTIS_PER_BOARD*multiplier + board*NUM_POTIS_PER_BOARD + poti;
}
byte potiIdentifier(byte board, byte poti){
  return potiIdentifier(board,poti,0);
}



void setMultiplexer(byte i){
  for(byte i = 0; i<3; i++){

  }
  digitalWrite(MULTIPLEXER_PINS[0], i&(0b00000001));
  digitalWrite(MULTIPLEXER_PINS[1], i&(0b00000010)); 
  digitalWrite(MULTIPLEXER_PINS[2], i&(0b00000100)); 
}


int readValueSimple(byte board, byte poti){
  return analogRead(INPUT_PINS[board]);
}

int readValueRunningAverage(byte board, byte poti){
  runningAverageValues[potiIdentifier(board,poti,runningAverage)] = analogRead(INPUT_PINS[board]);
  int value = 0;
  for(int i = 0; i<AVERAGE_SIZE; i++){
    value=value+runningAverageValues[potiIdentifier(board,poti,i)];
  }
  return value;
}


boolean valueChanged(byte board, byte poti, byte value){
  //Serial.println(String("board: ")+board+String("  poti: ")+poti+String("  value: ")+value+String("  id: ")+potiIdentifier(board,poti)+String("  old value: ")+values[potiIdentifier(board,poti)]);
  if(values[potiIdentifier(board,poti)] != value){
    values[potiIdentifier(board,poti)] = value;
    return true;
  }
  return false;
}

byte mapValue(int value){
  return map(value, 0, 1024*AVERAGE_SIZE, 127, 0);
}

void sendToPc(byte board, byte poti, byte value){
  pinMode(LED_BUILTIN_TX,OUTPUT);
  Serial.println(String(CC_NUMBER[potiIdentifier(board,poti)])+": "+String(value));
  midiEventPacket_t event = {0x0B, 0xB0 | CHANNEL, CC_NUMBER[potiIdentifier(board,poti)], value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
  pinMode(LED_BUILTIN_TX,INPUT);
}




void loop() {
   for(byte poti = 0; poti<NUM_POTIS_PER_BOARD; poti++){
      setMultiplexer(poti);
      for(byte board = 0; board < NUM_BOARDS; board++){
        byte value = mapValue(readValueRunningAverage(board, poti));
        if(valueChanged(board, poti, value)==true){
          sendToPc(board,poti, value);
        }
      }
   }  
   runningAverage = ((runningAverage+1)%AVERAGE_SIZE);           
}