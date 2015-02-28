/*
 pratipo.org GPL v3
 
 SIoT DOWN workshop+installation node, v0.1
 
 this is the firmware installed in the nodes/chairs
 meant for an arduino nano 328 + nrf24l01+ transceiver
 posible sensors: pressure, distance
 posible actuators: L293B, 2x 2n2222 (sound recording module)

 this software is based on the maniacbug starpin example for the RF24 library
 it uses the tmrh20 fork of such library http://tmrh20.github.io/RF24/
*/

// LIBS
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

///////////////////////////////////////////////////////////////////////////
//RADIO
RF24 radio(9,10);
// topology: one pipe per chair->master and one "multicast" pipe master->chairs
const uint64_t talking_pipes[5] =     { 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL };
const uint64_t broadcast_pipe[1] =        { 0x3A3A3A3AD2LL };
// address (defines role)
// The pong back unit takes address 1, and the rest are 2-6
uint8_t node_address = 0; // master is 0, chairs are 1,2,3,4,5

// ACTUATORS
//l293(motor)
#ifdef MOTOR 
  int menPin = 6;
  int mdir1Pin = 7;
  int mdir1Pin = 8;
  int mSpeedPin = 128;
#endif
//2N2222 transsistors: sound player
#ifdef TRANS1 
  int trans1Pin = 2;
#endif
#ifdef TRANS2 
  int trans2Pin = 3;
#endif

// SENSORS
//preassure (busy chair)
int presPin = 6;
int presThreshold = 512; // adjust acording to specific sensor

//HC-SR04: distance sensor
#ifdef DISTANCE 
  int triggerPin = A0;
  int echoPin = A1;
  long distance;
  long pulseLength;
#endif


// DATA STRUCTURES /////////////////////////////////////////////////////////
unsigned long ctime = 0;

struct systemData{
  unsigned long timeStamp;  // time in milliseconds
  byte chairs[5];       // busy state of each chair
  byte busyChairs;      // number of busy chairs
  // ... 
  //twitter?
}global_state;

struct payloadData{
  unsigned long timeStamp;  // time in milliseconds
  byte busy;
};

void updateChairs(){
  global_state.busyChairs = 0;
  for(int i=0; i<5; i++)
    global_state.busyChairs += global_state.chairs[i] ;
}

// SETUP /////////////////////////////////////////////////////////////////
void setup(void)
{
  // Print preamble
  Serial.begin(57600);
  printf_begin();

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false);

  if ( node_address == 0 ) { // master
    radio.openReadingPipe(1,talking_pipes[0]);
    radio.openReadingPipe(2,talking_pipes[1]);
    radio.openReadingPipe(3,talking_pipes[2]);
    radio.openReadingPipe(4,talking_pipes[3]);
    radio.openReadingPipe(5,talking_pipes[4]);
    
    radio.openWritingPipe(broadcast_pipe[0]);
  }
  else { 
    radio.openWritingPipe(talking_pipes[node_address-1]); // node's own pipe for private communciation withe master
    radio.openReadingPipe(1,broadcast_pipe[0]);
  }
  radio.startListening();
  radio.printDetails();
  printf("\n\r \n\r siot down\n\r");
  printf("adress: %i\n\r",node_address);
  
  for(int i=0; i<5; i++)
    global_state.chairs[i] = false;
}

// LOOP  /////////////////////////////////////////////////////////////////
void loop(void) {
  ctime = millis();
  
  // MASTER **************************************************************
  if ( node_address == 0 ) { // master
    uint8_t pipe_num;
    if ( radio.available(&pipe_num) ) {
      //READ
      payloadData data;
      radio.read( &data, sizeof(payloadData) );
      printf("got payload %lu from chair %i \n\r",data.busy, pipe_num);
      
      //UPDATE global state
      global_state.chairs[pipe_num] = data.busy;
      updateChairs();
    }
      //WRITE once a second 
    if (millis()-ctime > 1000){   
      radio.stopListening();  
      radio.write( &global_state, sizeof(systemData) );
      radio.startListening();
      printf("broadcasted (number of busy chairs %lu ) \n\r", global_state.busyChairs);
      
      ctime = millis();
    }
    
    // TWITTER ?!
    
  }
  // CHAIR **************************************************************
  else {
    if ( radio.available() ){
      // READ & UPDATE global state (by reference)
      radio.read( &global_state, sizeof(systemData) );
      printf("got global state: %lu \n\r",.busyChairs);
    }
      // WRITE once a second 
    if (millis()-ctime > 1000){  
      payloadData data;
      data.timeStamp = millis();
      data.busy = true;
      
      radio.stopListening();
      radio.write( &data, sizeof(payloadData) );
      radio.startListening();
      printf("now sending %lu payload \n\r", data.timeStamp);
      
      ctime = millis();
    }
    // KEEP INNER STATE
    //read pressure sensor
    // move motors
    // play sound
    // ...
  }
}

