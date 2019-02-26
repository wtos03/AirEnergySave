
 /* Copyright (c) 2018 Darryl Scott. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 * 
 * Based on the Code from Neil Kolban: https://github.com/nkolban/esp32-snippets/blob/master/hardware/infra_red/receiver/rmt_receiver.c
 * Based on the Code from pcbreflux: https://github.com/pcbreflux/espressif/blob/master/esp32/arduino/sketchbook/ESP32_IR_Remote/ir_demo/ir_demo.ino
 * Based on the Code from Xplorer001: https://github.com/ExploreEmbedded/ESP32_RMT
 */
/* This is a simple code to receive and then save IR received by an IR sensor connected to the ESP32 and then resend it back out via an IR LED  
 * Reason that I created this I could not find any complete examples of IR send and Received in the RAW format, I am hoping this code can then be used
 * to build on the existing IR libraries out there.
 *
 * 20/12/2018
 * Air con Remote too long. Cannot use irrecv in RMT driver ( Error RMT BUFFER FULL)
 * Use Interrupt to detect change on GPIO PIN and get different of time in micros 
 * for getting time period in micro second.
 *

 */

 
#include "ESP32_IR_Remote.h"

//you may increase this value on Arduinos with greater than 2k SRAM
#define maxLen 1000


const int RECV_PIN = 27; // pin on the ESP32
const int SEND_PIN = 25; // pin on the ESP32

volatile unsigned int x = 0; //Pointer thru irBuffer - volatile because changed by ISR



ESP32_IRrecv irrecv;

#define SendIRxTimes 2



volatile  unsigned int IRdata[maxLen]; //stores timings - volatile because changed by ISR
//unsigned int IRdata[maxLen]; //stores timings - 

void setup() {
  Serial.begin(115200);
//  irrecv.ESP32_IRrecvPIN(RECV_PIN,0);//channel 0 so it can use the full memory of the channels
  Serial.println("Initializing...");
  attachInterrupt(digitalPinToInterrupt(RECV_PIN), rxIR_Interrupt_Handler, CHANGE);//set up ISR for receiving IR signal

//  irrecv.initReceive();
  Serial.println("Init complete");
  Serial.println("Send an IR to Copy");
}


void loop() {
  unsigned int codeLen=0;

Serial.println(F("Press the button on the remote now - once only"));
  delay(5000); // pause 5 secs
  
  if (x) { //if a signal is captured
    Serial.println();
    Serial.print(F("Raw: (")); //dump raw header format - for library
    Serial.print((x - 1));
    Serial.print(F(") "));
    codeLen=(x-1);
     
    detachInterrupt(digitalPinToInterrupt(RECV_PIN));//stop interrupts & capture until finshed here
    for (int i = 1; i < x; i++) { //now dump the times
      unsigned int  tmp;
       tmp =   IRdata[i]  - IRdata[i-1];
       IRdata[i-1] = tmp;
      Serial.print(IRdata[i-1]);
      Serial.print(F(", "));
    }
    x = 0;
    Serial.println();
    Serial.println();
 
// Send IR
      if (codeLen > 6) { //ignore any short codes
        Serial.print("RAW code length: ");
        Serial.println(codeLen);
 //       irrecv.stopIR(); //uninstall the RMT channel so it can be reused for Receiving IR or send IR
        irrecv.ESP32_IRsendPIN(SEND_PIN,0);//channel 0 so it can use the full memory of the channels
        irrecv.initSend(); //setup the ESP32 to send IR code

        
        delay(1000);
        for(int i = 0;i<SendIRxTimes;i++){ //send IR code that was recorded
            irrecv.sendIR(IRdata,codeLen);
            delay(1000);
        }
      codeLen=0;
    irrecv.stopIR(); //uninstall the RMT channel so it can be reused for Receiving IR or send IR
      }
    }
    
 //Finish sending    
    attachInterrupt(digitalPinToInterrupt(RECV_PIN), rxIR_Interrupt_Handler, CHANGE);//re-enable ISR for receiving IR signal
    delay(5000);

}  

void rxIR_Interrupt_Handler() {
  if (x > maxLen) return; //ignore if irBuffer is already full
  IRdata[x++] = micros(); //just continually record the time-stamp of signal transitions

}






