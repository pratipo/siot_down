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
const uint64_t talking_pipes[5] =     { 
  0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL };
const uint64_t broadcast_pipe[1] =        { 
  0x3A3A3A3AD2LL };
// address (defines role)
// The pong back unit takes address 1, and the rest are 2-6
// master is 0, chairs are 1,2,3,4,5
// 1 -> gealous chair
// 2-> vaga
// 3 -> seductora
// 4 -> shy
// 5 -> chistosa
uint8_t node_address = 4; 

// ACTUATORS
//l293(motor)
int menPin = 6;
int mdir1Pin = 7;
int mdir2Pin = 8;
boolean up = true;
unsigned long timer;

//2N2222 transsistors: sound player
int trans1Pin = 2;
int trans2Pin = 3;

// SENSORS
int cycles = 0;

//preassure (busy chair)
int presPin = A7;
int presThreshold[6] = { 0, 300, 270, 350, 140, 500 }; // adjusted to specific sensor per chair
unsigned long pres;
boolean busy;
boolean change;

//HC-SR04: distance sensor
int triggerPin = A0;
int echoPin = A1;

// DATA STRUCTURES /////////////////////////////////////////////////////////
unsigned long settime = 0;

struct systemData{
  unsigned long timeStamp;  // time in milliseconds
  byte chairs[5];       // busy state of each chair
  byte busyChairs;      // number of busy chairs
  // ... 
  //twitter?
}
global_state;

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
void setup(void){
  //PINS
  pinMode(menPin, OUTPUT);
  pinMode(mdir1Pin, OUTPUT);
  pinMode(mdir2Pin, OUTPUT);
  
  pinMode(trans1Pin, OUTPUT);
  pinMode(trans2Pin, OUTPUT);
  
  pinMode(presPin, INPUT);
  
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
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

  for(int i=0; i<5; i++){
    global_state.chairs[i] = false;
  }
  settime = millis();
}

// AUX FUNCTIONS

void controlMotor (int speed, boolean dir){
  analogWrite(menPin, speed);
  digitalWrite(mdir1Pin, dir);
  digitalWrite(mdir2Pin, dir);  
}

long getDistance(){
  digitalWrite(triggerPin,LOW);   
  delayMicroseconds(5);
  digitalWrite(triggerPin, HIGH); 
  delayMicroseconds(10);
  long distance=pulseIn(echoPin, HIGH)*0.017;
  printf("distance: %l cm\n\r", distance);
  return distance; 
}

// LOOP  /////////////////////////////////////////////////////////////////
void loop(void) {
  //settime = millis();

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
    if (millis()-settime > 1000){   
      radio.stopListening();  
      radio.write( &global_state, sizeof(systemData) );
      radio.startListening();
      printf("broadcasted (number of busy chairs %lu ) \n\r", global_state.busyChairs);

      settime = millis();
    }

    // TWITTER
    // Serial.println();

  }
  // CHAIR **************************************************************
  else {
    if ( radio.available() ){
      // READ & UPDATE global state (by reference)
      radio.read( &global_state, sizeof(systemData) );
      printf("got global state: %lu \n\r", global_state.busyChairs);
    }

    // KEEP INNER STATE
    //read pressure sensor
    cycles++;
    pres += analogRead(presPin);

    //--------------------------------------------------------
    if ( node_address == 1 ) { // gelous chair
      if (!busy){
        if (!random(3)){ //random( frq) -> prob to trigger
          //boolean a = (boolean)random(1);
          controlMotor(127 + global_state.busyChairs*32, LOW);
        }
        else{
          controlMotor(0, LOW); // OFF
        }
      }
    }
    //--------------------------------------------------------
    else if ( node_address == 2) { // lazy chair

      if (change){
        if ((busy && up) || (!busy && !up))
          timer = millis(); // load timer
        else
          timer = 0;
      }
      if ( timer && (millis()-timer>10000) ){
        if (up){
          // move down
          controlMotor(255, HIGH);
          delay(1000);
        }
        else{
          // move up
          controlMotor(255, LOW);
          delay(1000);
        }
        // off
        controlMotor(0, LOW); 
        timer = 0;
        up = !up; // flip state
      }
    }
    //--------------------------------------------------------
    else if ( node_address == 3 ) { // casanova chair
      // search for people when not busy
      if (!busy){
        long distance = getDistance();
        if (distance<80) {
        }
        else if (distance<150){ // very close
          // whisle
          if (!timer){
            digitalWrite(2, HIGH);
            delay(100);
            digitalWrite(2, LOW);
            timer = millis();
          }
          else if (millis()-timer>30000){ // turn off whisling for 30 seconds
            timer = 0;
          }
        }
        else if (!random(3)){
          controlMotor(225, random(1));
          delay(random(1000, 3000));
        }
      }
    }
    //--------------------------------------------------------
    else if ( node_address == 4 ) { // shy chair
      // avoid people
      if (!busy){
        long distance = getDistance();
        if (distance>80) { 
          // avoid eye contact
          if (!timer){
            controlMotor((random(64)+96), random(1));
            timer = millis();
          }
          else if (millis()-timer>1000){ // turn off whisling for 30 seconds
            controlMotor(0, LOW);
            timer = 0;
          }
        }
      }
    }
    //--------------------------------------------------------
    else if ( node_address == 5 ) { // funny chair
      if (busy && change && global_state.busyChairs>2){
        if (!timer){
          digitalWrite(2, HIGH);
          delay(100);
          digitalWrite(2, LOW);
          timer = millis();
        }
        else if (millis()-timer > 30000){
          timer = 0;         
        }
      }   
    }

    //*******************************************************************
    // CYCLE
    if (millis()-settime > 1000){  
      settime = millis();

      // update busy state
      pres = pres/cycles;


      if (pres>presThreshold[node_address]){
        if (!busy) {
          change = true;
          busy = true; 
        }
        else{
          change = false;
        } 
      }
      else{
        if (busy) {
          change = true;
          busy = false;
        }
        else{
          change = false;
        }
      }
      cycles = 0;
      pres = 0;

      // WRITE once a second payloadData data;
      payloadData data;

      data.timeStamp = millis();
      data.busy = busy;

      radio.stopListening();
      radio.write( &data, sizeof(payloadData) );
      radio.startListening();
      printf("sent %lu payload \n\r", data.timeStamp);
    }

  }

  delay(10);
}






