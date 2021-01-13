/*
INDEX PNP
Stephen Hawes 2021

This firmware is intended to run on the Index PNP feeder main board. 
It is meant to index component tape forward, while also intelligently peeling the film covering from said tape.
When the feeder receives a signal from the host, it indexes a certain number of 'ticks' or 4mm spacings on the tape
(also the distance between holes in the tape)

#ifdef DEBUG
  Serial.println("INFO - debug message here");
#endif

*/

#include "define.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <OneWire.h>

//
//global variables
//
unsigned long startMillis;  
unsigned long currentMillis;
bool film_tension_flag = false;
byte addr = 0;

HardwareSerial ser(PA10, PA9);

OneWire ds(ONE_WIRE);

//-------
//SETUP
//-------
void setup() {

  #ifdef DEBUG
    Serial.println("INFO - Feeder starting up...");
  #endif

  //setting pin modes
  pinMode(DE, OUTPUT);
  pinMode(_RE, OUTPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);

  pinMode(FILM_TENSION, INPUT_PULLUP);

  pinMode(OPTO_SIG, INPUT_ANALOG);

  pinMode(DRIVE1, OUTPUT);
  pinMode(DRIVE2, OUTPUT);
  pinMode(PEEL1, OUTPUT);
  pinMode(PEEL2, OUTPUT);

  //init led blink
  for(int i = 0;i <= 5;i++){
    if(i%2 == 0){
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      delay(200);
    }
    else{
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      delay(200);
    }
  }

  //setting initial pin states
  digitalWrite(DE, LOW);
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);

  //Starting rs-485 serial
  ser.begin(9600);

  byte floor_addr = read_floor_addr();

  if(floor_addr == 0x00){ //floor 1 wire eeprom has not been programmed
    if(write_floor_addr()){
      //programmed successfully
    }
    else{
      //programming failed
    }
  }
  else{ //successfully read address from eeprom
    addr = floor_addr;
  }

}

byte read_floor_addr(){

}

byte write_floor_addr(){
  //programms a feeder floor. 
  //successful programming returns the address programmed
  //failed program returns 0x00

}

void send(byte addr, byte data){
  //flip RTS pin to send
  digitalWrite(DE, HIGH);

  //write address, then data bytes
  ser.write(addr);
  ser.write(data);

  //bring RTS pin back to listen mode
  digitalWrite(DE, LOW);


}



void index(int pip_num, bool direction){

  for(int i = 0; i < pip_num; i++){

    if(direction){ //if moving forward

      // first threshold
      while(analogRead(OPTO_SIG)<300){

        #ifdef OPTO_DEBUG
          ser.println(analogRead(OPTO_SIG));
        #endif

        analogWrite(DRIVE1, 200);
        analogWrite(DRIVE2, 0);
        delay(15);
        analogWrite(DRIVE1, 0);
        delay(50);
      
      }

      // second threshold
      while(analogRead(OPTO_SIG)>200){
        ser.println(analogRead(OPTO_SIG));
        analogWrite(DRIVE1, 200);
        analogWrite(DRIVE2, 0);
        digitalWrite(LED1, LOW);
        delay(15);
        analogWrite(DRIVE1, 0);
        delay(50);
      }
    
      //third threshold
      while(analogRead(OPTO_SIG)<250){
        ser.println(analogRead(OPTO_SIG));
        analogWrite(DRIVE1, 200);
        analogWrite(DRIVE2, 0);
        digitalWrite(LED1, LOW);
        delay(15);
        analogWrite(DRIVE1, 0);
        delay(50);
      }

      while(digitalRead(FILM_TENSION)){//if film tension switch not clicked
        //then spin motor to wind film
        analogWrite(PEEL2, 100);
        analogWrite(PEEL1, 0);
      }
            
      analogWrite(PEEL2, 0);
      analogWrite(PEEL1, 0);
             
    }
    else{ //if going backward

      //unspool some film to give the tape slack. imprecise amount because we retention later
      analogWrite(PEEL1, 100);
      analogWrite(PEEL2, 0);
      delay(400);
      analogWrite(PEEL1, 0);
      analogWrite(PEEL2, 0);

      // first threshold
      while(analogRead(OPTO_SIG)<300){

        #ifdef OPTO_DEBUG
          ser.println(analogRead(OPTO_SIG));
        #endif

        analogWrite(DRIVE1, 0);
        analogWrite(DRIVE2, 250);
        delay(20);
        analogWrite(DRIVE2, 0);
        delay(50);
      
      }

      // second threshold
      while(analogRead(OPTO_SIG)>200){
        ser.println(analogRead(OPTO_SIG));
        analogWrite(DRIVE1, 0);
        analogWrite(DRIVE2, 250);
        digitalWrite(LED1, LOW);
        delay(20);
        analogWrite(DRIVE2, 0);
        delay(50);
      }
    
      //third threshold
      while(analogRead(OPTO_SIG)<250){
        ser.println(analogRead(OPTO_SIG));
        analogWrite(DRIVE1, 0);
        analogWrite(DRIVE2, 250);
        digitalWrite(LED1, LOW);
        delay(20);
        analogWrite(DRIVE2, 0);
        delay(50);
      }

      while(digitalRead(FILM_TENSION)){//if film tension switch not clicked
        //then spin motor to wind film
        analogWrite(PEEL2, 100);
        analogWrite(PEEL1, 0);
      }
            
      analogWrite(PEEL2, 0);
      analogWrite(PEEL1, 0);
    }
  }
}

void listen(){
  if (ser.available() > 0) {
    digitalWrite(LED1, LOW);
    delay(100);
    digitalWrite(LED1, HIGH);
    // read the incoming byte:
    byte req_addr = ser.read();

    //check if byte is this feeder's id
    if(addr == req_addr){
      byte command = ser.read();
      ser.write(command);

      if(command==0b01000110){
        index(1, true);
      }
      else if(command==0b01000010){
        index(1, false);
      }
  
      
    }
    else{
      //dump serial buffer and ignore the rest of the transmission
    }
  }

}

//------
//MAIN CONTROL LOOP
//------

void loop() {

// Checking SW1 status to go forward

  if(!digitalRead(SW1)){
    delay(LONG_PRESS_DELAY);

    if(!digitalRead(SW1)){
      //we've got a long press, lets go speedy
      analogWrite(DRIVE1, 255);
      analogWrite(DRIVE2, 0);
      
      while(!digitalRead(SW1)){
        //do nothing
      }
    
      analogWrite(DRIVE1, 0);
      analogWrite(DRIVE2, 0);
    }
    else{
      index(1, true);
    }
    
    
  }

// Checking SW2 status to go backward

  if(!digitalRead(SW2)){
    delay(LONG_PRESS_DELAY);

    if(!digitalRead(SW2)){
      //we've got a long press, lets go speedy
      analogWrite(DRIVE1, 0);
      analogWrite(DRIVE2, 255);
      
      while(!digitalRead(SW2)){
        //do nothing
      }
    
      analogWrite(DRIVE1, 0);
      analogWrite(DRIVE2, 0);
    }
    else{
      index(1, false);
    }
    
}


//listening on rs-485 for a command

  listen();


// end main loop
}
