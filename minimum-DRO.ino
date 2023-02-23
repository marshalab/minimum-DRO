/*
 minimum DRO

 iGaging Digital Scales Controller V0.1a
 Created 17 Junary 2023
 Update 17 Junary 2023
 Copyright (C) 2023 Arkhipov Aleksandr, https://github.com/marshalab

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Version 0.1 alfa
  
Shahe scale - inverter level converter K561LN2 - esp32 
 + level converter for 5V TM1637 6-digits x3
 + keypad 4x4 PCF8574 - X0 Z0 ABS/INC Enter
 + switch Radius/Diametr
 + switch RPM/Speed
 + 22 mm power button
 + tachometr input
 + probe input + Buzzer = calibrator
 + 6 channel memory
 + mm / inch
 + reverse scale measurement
 
3D print case with magnets/GoPro mount
 - print table work mode
 - power in 7-24V

future:
 BT SPP for Android TouchDRO
 WiFi - MGX3D_EspDRO
WebSockets broadcasting: allows building simple, low latency clients (10 lines of code Python sample PyDRO included as well as JavaScript/HTML jsDRO )
Built-in web server: allows reading the data from browser/phone/tablet without any additional apps. Big display!
REST APIs and mDNS support: to build thin client apps easily
USB serial: for running without WiFi

 Shahe scale = iGaging protocol 21 bit
 https://aliexpress.ru/item/32852522653.html?spm=a2g2w.orderdetail.0.0.41b54aa69ZWbm9&sku_id=12000027074363870
 
*/

#include "TM1637.h"
//    FILE: TM1637_float.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo TM1637 library
//     URL: https://github.com/RobTillaart/TM1637_RT

#include "Wire.h"
#include "I2CKeyPad.h"
//    FILE: I2Ckeypad_keymap.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo key mapping
//     URL: https://github.com/RobTillaart/I2CKeyPad
// PCF8574    pin p0-p3 rows    pin p4-p7 columns  4x4 or smaller keypad.

#include <EEPROM.h>

#include <Bounce2.h>

//--- START CONFIGURATION PARAMETERS ---

// General Settings
#define UART_BAUD_RATE 115200        //  Set this so it matches the BT module's BAUD rate 
#define UPDATE_FREQUENCY 24       //  Frequency in Hz (number of timer per second the scales are read and the data is sent to the application)
#define TACH_UPDATE_FREQUENCY 4     //  Max Frequency in Hz (number of timer per second) the tach output is sent to the application

#define KEYPAD_SDA_PIN 21
#define KEYPAD_SCL_PIN 22
#define KEYPAD_ADDRESS 0x20

#define BEEP_PIN  02

// DRO config (if axis is not connected change in the corresponding constant value from "1" to "0")
#define SCALE_X_ENABLED 1
#define SCALE_Z_ENABLED 1

// I/O ports config (change pin numbers if DRO, Tach sensor or Tach LED feedback is connected to different ports)
#define SCALE_X_CLK_PIN 33
#define SCALE_X_DATA_PIN 26
#define SCALE_Z_CLK_PIN 35
#define SCALE_Z_DATA_PIN 17

#define SCALE_INTERVAL_UPDATE 50

// Tach config (if Tach is not connected change in the corresponding constant value from "1" to "0")
#define TACH_ENABLED 1
#define INPUT_TACH_PIN 39 //SVN

// Tach RPM config
#define MIN_RPM_DELAY 2000        // 1.2 sec calculates to low range = 50 rpm.

boolean tachoProbe = LOW;
long tachoTime = 0;

// Touch probe config (if Touch Probe is not connected change in the corresponding constant value from "1" to "0")
#define PROBE_ENABLED 1
#define INPUT_PROBE_PIN 36 //SVP

// Touch probe invert signal config
#define PROBE_INVERT 1          // Touch Probe signal inversion: Open = Input pin is Low; Closed = Input pin is High

boolean probeWire = LOW;


#define  SWTCH1PIN  34
byte Switch1State = LOW;
Bounce Switch1Bouncer = Bounce();

#define  SWTCH2PIN  16
byte Switch2State = LOW;
Bounce Switch2Bouncer = Bounce();

#define TM1637_1_DATA_PIN 27
#define TM1637_1_CLK_PIN 25
#define TM1637_2_DATA_PIN 19
#define TM1637_2_CLK_PIN 18
#define TM1637_3_DATA_PIN 23
#define TM1637_3_CLK_PIN 5

//--- END OF CONFIGURATION PARAMETERS ---

//---DO NOT CHANGE THE CODE BELOW UNLESS YOU KNOW WHAT YOU ARE DOING ---

/* iGaging Clock Settings (do not change) */
#define SCALE_CLK_PULSES 21        // iGaging and Accuremote scales use 21 bit format
#define SCALE_CLK_FREQUENCY 9000    // iGaging scales run at about 9-10KHz
#define SCALE_CLK_DUTY 20       // iGaging scales clock run at 20% PWM duty (22us = ON out of 111us cycle)

//--- END DEFINE ---

//Shahe DRO scale
//This code is more general, written for Arduino (I tested it with Nano)
int dro_bits1[24];        // For storing the data bit. bit_array[0] = data bit 1 (LSB), bit_array[23] = data bit 24 (MSB).
char str1[7]; //Array for the char "string"
unsigned long droTimer1 = 0; //update frequency (the DRO is checked this often)
int clk1 = SCALE_X_CLK_PIN; //Blue
int data1 = SCALE_X_DATA_PIN; //Red
float rawValueX = 0.0; //raw conversion value (bit to mms)
float temporaryValueX = 0.0; //temporary storage to be able to register a change in the value
float resultValueX= 0.0; //final result value: conversion value - tare value (if there is any taring)
#define INVERTIN  1
#define MAGICCONSTANT 1048575

int dro_bits2[24];        // For storing the data bit. bit_array[0] = data bit 1 (LSB), bit_array[23] = data bit 24 (MSB).
char str2[7]; //Array for the char "string"
unsigned long droTimer2 = 0; //update frequency (the DRO is checked this often)
int clk2 = SCALE_Z_CLK_PIN; //Blue
int data2 = SCALE_Z_DATA_PIN; //Red
float rawValueZ = 0.0; //raw conversion value (bit to mms)
float temporaryValueZ = 0.0; //temporary storage to be able to register a change in the value
float resultValueZ = 0.0; //final result value: conversion value - tare value (if there is any taring)

long resultRPM_T = 0;
long resultLine_T = 0;

int performanceDRO = 0;
static int perfomanceTimer = 0;

//LCD TM1637
TM1637 TM1;
TM1637 TM2;
TM1637 TM3;
  
//keypad 4x4 PCF8574
I2CKeyPad keyPad(KEYPAD_ADDRESS);
char keymap[19] = "D#0*C987B654A321NF";  // N = NoKey, F = Fail

struct EepromData {
  byte memoryChX;        //текущий канал 0-7
  byte memoryChZ;        //текущий канал 0-7
  float tareX[6]={};
  float tareZ[6]={};
  boolean plus_minus_X;         // +- X
  boolean plus_minus_Z;         // +- Z
  byte mm_inch;         //0-mm 1-inch
  byte crc;  // байт crc
};

  EepromData setupSet;

void readEncoder1()
{ 
  
  //This function reads the encoder
  //I added a timer if you don't need high update frequency

  if (SCALE_X_ENABLED)
  if (millis() - droTimer1 > SCALE_INTERVAL_UPDATE) //if 1 s passed we start to wait for the incoming reading
  {
    rawValueX = 0; //set it to zero, so no garbage will be included in the final conversion value

    //This part was not shown in the video because I added it later based on a viewer's idea
    unsigned long syncTimer = 0; //timer for syncing the readout process with the DRO's clock signal
    bool synchronized = false; // Flag that let's the code know if the clk is synced
    while (synchronized == false)
    {
      syncTimer = millis(); //start timer
      while (digitalRead(clk1) == !INVERTIN) {} //wait until the clk goes low
      //Time between the last rising edge of CLK and the first falling edge is 115.7 ms
      //Time of the "wide high part" that separates the 4-bit parts: 410 us

      if (millis() - syncTimer > 5) //if the signal has been high for more than 5 ms, we know that it has been synced
      { //with 5 ms delay, the code can re-check the condition ~23 times so it can hit the 115.7 ms window
        synchronized = true;
      }
      else
      {
        synchronized = false;
      }
    }

    for (int i = 0; i < 23; i++) //We read the whole data block - just for consistency
    {
      while (digitalRead(clk1) == INVERTIN) {} // wait for the rising edge

      dro_bits1[i] = digitalRead(data1);
      //Print the data on the serial
//      Serial.print(dro_bits1[i]);
//      Serial.print(" ");

      while (digitalRead(clk1) == !INVERTIN) {} // wait for the falling edge
    }
//    Serial.println(" ");

    //Reconstructing the real value
    for (int i = 0; i < 20; i++) //we don't process the whole array, it is not necessary for our purpose
    {
      rawValueX = rawValueX + (pow(2, i) * dro_bits1[i]);
      //Summing up all the 19 bits.
      //Essentially: 1*[i] + 2*[i] + 4*[i] + 8*[i] + 16 * [i] + ....
    }

    if (dro_bits1[20] == 1)
    {
      //don't touch the value (stays positive)
//      Serial.println("Positive ");
    }
    else
    {
      rawValueX = -1 * rawValueX; // convert to negative
//      Serial.println("Negative ");
      rawValueX = rawValueX + 2 * MAGICCONSTANT;
    }

    performanceDRO++;

    rawValueX = (rawValueX / 100.0); //conversion to mm
    //Division by 100 comes from the fact that the produced number is still an integer (e.g. 9435) and we want a float
    //The 100 is because of the resolution (0.01 mm). x/100 is the same as x*0.01.

    //The final result is stored in a separate variable where the tare is subtracted
    //We need a separate variable because the converted value changes "on its own scale".
    if (setupSet.mm_inch) {
      resultValueX = (rawValueX - setupSet.tareX[setupSet.memoryChX]) * 0.0393701;
    }
    else{
      resultValueX = rawValueX - setupSet.tareX[setupSet.memoryChX];
    }
    if(setupSet.plus_minus_X) resultValueX = -resultValueX;

    //Dump everything on the serial
//    Serial.print("Raw reading: ");
//    Serial.print(rawValueX);
//    Serial.print(" Tare value: ");
//    Serial.print(tareValue1);
//    Serial.print(" Result after taring: ");
//    Serial.print(resultValueX);

    droTimer1 = millis();
  }
}

void readEncoder2()
{ 
  //This function reads the encoder
  //I added a timer if you don't need high update frequency

  if (SCALE_Z_ENABLED)
  if (millis() - droTimer2 > SCALE_INTERVAL_UPDATE) //if 1 s passed we start to wait for the incoming reading
  {
    rawValueZ = 0; //set it to zero, so no garbage will be included in the final conversion value

    //This part was not shown in the video because I added it later based on a viewer's idea
    unsigned long syncTimer = 0; //timer for syncing the readout process with the DRO's clock signal
    bool synchronized = false; // Flag that let's the code know if the clk is synced
    while (synchronized == false)
    {
      syncTimer = millis(); //start timer
      while (digitalRead(clk2) == !INVERTIN) {} //wait until the clk goes low
      //Time between the last rising edge of CLK and the first falling edge is 115.7 ms
      //Time of the "wide high part" that separates the 4-bit parts: 410 us

      if (millis() - syncTimer > 5) //if the signal has been high for more than 5 ms, we know that it has been synced
      { //with 5 ms delay, the code can re-check the condition ~23 times so it can hit the 115.7 ms window
        synchronized = true;
      }
      else
      {
        synchronized = false;
      }
    }

    for (int i = 0; i < 23; i++) //We read the whole data block - just for consistency
    {
      while (digitalRead(clk2) == INVERTIN) {} // wait for the rising edge

      dro_bits2[i] = digitalRead(data2);
      //Print the data on the serial
//      Serial.print(dro_bits2[i]);
//      Serial.print(" ");

      while (digitalRead(clk2) == !INVERTIN) {} // wait for the falling edge
    }
//    Serial.println(" ");

    //Reconstructing the real value
    for (int i = 0; i < 20; i++) //we don't process the whole array, it is not necessary for our purpose
    {
      rawValueZ = rawValueZ + (pow(2, i) * dro_bits2[i]);
      //Summing up all the 19 bits.
      //Essentially: 1*[i] + 2*[i] + 4*[i] + 8*[i] + 16 * [i] + ....
    }

    if (dro_bits2[20] == 1)
    {
      //don't touch the value (stays positive)
//      Serial.println("Positive ");
    }
    else
    {
      rawValueZ = -1 * rawValueZ; // convert to negative
//      Serial.println("Negative ");

      rawValueZ = rawValueZ + 2 * MAGICCONSTANT;
    }

    rawValueZ = (rawValueZ / 100.0); //conversion to mm
    //Division by 100 comes from the fact that the produced number is still an integer (e.g. 9435) and we want a float
    //The 100 is because of the resolution (0.01 mm). x/100 is the same as x*0.01.

    //The final result is stored in a separate variable where the tare is subtracted
    //We need a separate variable because the converted value changes "on its own scale".
    if (setupSet.mm_inch) {
      resultValueZ = (rawValueZ - setupSet.tareZ[setupSet.memoryChZ]) * 0.0393701;
    }
    else{
      resultValueZ = rawValueZ - setupSet.tareZ[setupSet.memoryChZ];
    }
    if(setupSet.plus_minus_Z) resultValueZ = -resultValueZ;
    
    //Dump everything on the serial
//    Serial.print("Raw reading: ");
//    Serial.print(rawValueZ);
//    Serial.print(" Tare value: ");
//    Serial.print(tareValue2);
//    Serial.print(" Result after taring: ");
//    Serial.print(resultValueZ);
//    Serial.println(" ");

    droTimer2 = millis();
  }
}

void clearEEPROM() {
  setupSet = {};
}

void saveEEPROM() {
  // расчёт CRC (без последнего байта)
  byte crc = crc8((byte*)&setupSet, sizeof(setupSet) - 1);
  // пакуем в посылку
  setupSet.crc = crc;
    
  EEPROM.put(0, setupSet);   // поместить в EEPROM по адресу 0
  Serial.println(" EEPROM save OK");
  Serial.printf("ChX%i, ChZ%i, TareX=%.2f, TareZ=%.2f, X-%s Z-%s\n", setupSet.memoryChX+1, setupSet.memoryChZ+1, setupSet.tareX[setupSet.memoryChX], setupSet.tareZ[setupSet.memoryChZ], setupSet.plus_minus_X ? "direct" : "invert", setupSet.plus_minus_Z ? "direct" : "invert");
//  byte setupSet.memoryChX;        //текущий канал 0-7
//  byte setupSet.memoryChZ;        //текущий канал 0-7
//  float setupSet.tareX[6]={};
//  float setupSet.tareZ[6]={};
//  byte setupSet.X_mm_inch;         //0-mm 1-inch
//  byte setupSet.Z_mm_inch;         //0-mm 1-inch  
//  byte setupSet.crc;  // байт crc
  EEPROM.commit();
}

byte loadEEPROM() {
  // читаем точно так же, как писали
  EEPROM.get(0, setupSet);   // прочитать из адреса 0

//  byte crc = crc8((byte*)&setupSet, sizeof(setupSet)); // считаем crc посылки полностью
//  if (crc == 0) {
      // данные верны
      Serial.println(" EEPROM load OK");
      Serial.printf("ChX%i, ChZ%i, TareX=%.2f, TareZ=%.2f, X-%s Z-%s\n", setupSet.memoryChX+1, setupSet.memoryChZ+1, setupSet.tareX[setupSet.memoryChX], setupSet.tareZ[setupSet.memoryChZ], setupSet.plus_minus_X ? "direct" : "invert", setupSet.plus_minus_Z ? "direct" : "invert");
      return 1;
//  } else {
//      // данные повреждены
//      Serial.println(" EEPROM load error");
//      clearEEPROM();
//      saveEEPROM();
//      return 0;
//  }
}

void setup() 
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  EEPROM.begin(100); 
  pinMode(BEEP_PIN, OUTPUT);

  Switch1Bouncer.attach( SWTCH1PIN ,  INPUT_PULLUP ); // USE INTERNAL PULL-UP
  Switch1Bouncer.interval(5); // interval in ms
  Switch2Bouncer.attach( SWTCH2PIN ,  INPUT_PULLUP ); // USE INTERNAL PULL-UP
  Switch2Bouncer.interval(5); // interval in ms
  
  Wire.begin();
  Wire.setClock(400000);
  // i2c scanner
  byte error, address;
  int nDevices = 0;
  Serial.println("Scanning for I2C devices ...");
  for(address = 0x01; address < 0x7f; address++){
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0){
      Serial.printf("I2C device found at address 0x%02X\n", address);
      tone(BEEP_PIN, 1000,500);
      nDevices++;
    } else if(error != 2){ 
      Serial.printf("Error %d at address 0x%02X\n", error, address);  
      tone(BEEP_PIN, 300,1000);      
    }
  }
  if (nDevices == 0){
    Serial.println("No I2C devices found");
    tone(BEEP_PIN, 300,1000);
  }
  if (keyPad.begin(KEYPAD_SDA_PIN, KEYPAD_SCL_PIN) == false)
  {
    Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
    tone(BEEP_PIN, 300,1000);
    while (1);
  }  
  keyPad.loadKeyMap(keymap);

  TM1.begin(TM1637_1_CLK_PIN, TM1637_1_DATA_PIN);       //  clockpin, datapin
  TM2.begin(TM1637_2_CLK_PIN, TM1637_2_DATA_PIN);       //  clockpin, datapin
  TM3.begin(TM1637_3_CLK_PIN, TM1637_3_DATA_PIN);       //  clockpin, datapin
  TM1.setBrightness(7);
  TM2.setBrightness(7);
  TM3.setBrightness(7);

  pinMode(SCALE_X_CLK_PIN, INPUT_PULLUP);
  pinMode(SCALE_X_DATA_PIN, INPUT_PULLUP);
  pinMode(SCALE_Z_CLK_PIN, INPUT_PULLUP);
  pinMode(SCALE_Z_DATA_PIN, INPUT_PULLUP);    

  pinMode(INPUT_TACH_PIN, INPUT);
  pinMode(INPUT_PROBE_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_TACH_PIN), tachoIsr, RISING);
  attachInterrupt(digitalPinToInterrupt(INPUT_PROBE_PIN), probreIsr, RISING);
  
  if (!loadEEPROM()) tone(BEEP_PIN, 200,1000);
}
// -- END setup() --

  #define MODEWORK 0 //modeEditZ
  #define MODEEDITX 2 //modeEditX
  #define MODEEDITZ 3 //modeEditZ
  byte modeSet = MODEWORK;
                        //modeWork  -1 
                        //modeEditX -2 ввод фактического значения X, выбор канала C
                        //modeEditZ -3 ввод фактического значения Z, выбор канала C 

  float inputValueX = 0;
  float inputValueZ = 0;
String inputString = "";
char buff1[20];
char buff[20];
char buff2[20];
  
void loop() {
  uint32_t now = millis();
  
  //SWITCHES
  Switch1Bouncer.update();
  Switch1State = Switch1Bouncer.read();

  Switch2Bouncer.update();
  Switch2State = Switch2Bouncer.read();

  //DISPLAY-MENU
  switch (modeSet) {
    case MODEWORK: //modeWork
      TM1.setBrightness(7);
      TM2.setBrightness(7);
      TM3.setBrightness(7);    
      if ( Switch1State == HIGH )          //Switch Radius / Diametr
        TM1.displayFloat(resultValueX,2);   //Radius
      else
        TM1.displayFloat(resultValueX*2,2);     //Diametr
      TM2.displayFloat(resultValueZ,2);
      if ( Switch2State == HIGH )          //RPM / SPEED
        if (setupSet.mm_inch) sprintf(buff1,"L.%4d", resultLine_T);
        else sprintf(buff1,"L %4d", resultLine_T);
      else
        if (setupSet.mm_inch) sprintf(buff1,"R.%4d", resultRPM_T);
        else sprintf(buff1,"R %4d", resultRPM_T);
      TM3.displayPChar(buff1);      
      break;
    case MODEEDITX: //modeEditX
      TM1.setBrightness(7);
      TM2.setBrightness(1);
      TM3.setBrightness(7);
      if(setupSet.plus_minus_X) temporaryValueX = -temporaryValueX;
      TM1.displayFloat(temporaryValueX,2);     //only Diametr input
      TM2.displayFloat(resultValueZ,2);
      switch(setupSet.memoryChX+1){
        case 1: sprintf(buff,"_     "); break;
        case 2: sprintf(buff," _    "); break;
        case 3: sprintf(buff,"  _   "); break;
        case 4: sprintf(buff,"   _  "); break;
        case 5: sprintf(buff,"    _ "); break;
        case 6: sprintf(buff,"     _"); break;
      }
      TM3.displayPChar(buff);   
      break;
    case MODEEDITZ: //modeEditZ
      TM1.setBrightness(1);
      TM2.setBrightness(7);
      TM3.setBrightness(7);
      if ( Switch1State == HIGH ) {         //Switch Radius / Diametr
        TM1.displayFloat(resultValueX,2);   //Radius
      }else
        TM1.displayFloat(resultValueX*2,2);     //Diametr
      if(setupSet.plus_minus_Z) temporaryValueZ = -temporaryValueZ;
      TM2.displayFloat(temporaryValueZ,2);
      switch(setupSet.memoryChZ+1){
        case 1: sprintf(buff,"_     "); break;
        case 2: sprintf(buff," _    "); break;
        case 3: sprintf(buff,"  _   "); break;
        case 4: sprintf(buff,"   _  "); break;
        case 5: sprintf(buff,"    _ "); break;
        case 6: sprintf(buff,"     _"); break;
      }
      TM3.displayPChar(buff);   
      break;
    default:  //modeWork
      // выполняется, если не выбрана ни одна альтернатива
      // default необязателен
      if ( Switch1State == HIGH ) {         //Switch Radius / Diametr
        TM1.displayFloat(resultValueX,2);   //Radius
      }else
        TM1.displayFloat(resultValueX*2,2);     //Diametr
      TM2.displayFloat(resultValueZ,2);
      if ( Switch2State == HIGH )          //RPM / SPEED
        if (setupSet.mm_inch) sprintf(buff1,"L.%4d", resultLine_T);
        else sprintf(buff1,"L %4d", resultLine_T);
      else
        if (setupSet.mm_inch) sprintf(buff1,"R.%4d", resultRPM_T);
        else sprintf(buff1,"R %4d", resultRPM_T);
      TM3.displayPChar(buff1);      
  }
  
  //KEYPAD
  static boolean isUnpressed = LOW;
  static uint32_t isUnpressedTimeOut = 0;
  static uint32_t lastTimeKeyPressed = 0;
  static unsigned char lastCodeKeyPressed[6] = {};
  static byte keypad_press_count = 0;
  static byte keypad_C_press_count = 0;
  #define KEYPAD_1_PRESS_TIME 25
  #define KEYPAD_2ST_PRESS_TIMEOOUT 700
  static uint32_t isPressedTimeOut = 0;
  #define KEYPAD_LONG_PRESS_TIME 1000
  #define KEYPAD_UNPRESS_TIME 15
  static char memoryCh = 0;

  if (!keyPad.isPressed())
  {       //isUnPressed
    if (now - isUnpressedTimeOut >= KEYPAD_UNPRESS_TIME)    
    {
      isUnpressedTimeOut = now;
      isUnpressed = HIGH;
    }
  }
  else
  {  //isPressed
    if (now - lastTimeKeyPressed >= KEYPAD_LONG_PRESS_TIME && isUnpressed == LOW)
    {     // LONG press - A/B - mm/inch
      lastTimeKeyPressed = now;
      keypad_press_count = 0;
      lastCodeKeyPressed[0] = 255;
      Serial.println(" _|_|_|_ ");
      tone(BEEP_PIN, 300,500);

      if (modeSet == MODEEDITX && keyPad.getChar()=='A') 
        {                     //смена +- для редактируемой оси 
          setupSet.plus_minus_X = !setupSet.plus_minus_X;
          saveEEPROM();
        }
      if (modeSet == MODEEDITZ && keyPad.getChar()=='B') 
        {                     //смена +- для редактируемой оси 
          setupSet.plus_minus_Z = !setupSet.plus_minus_Z;
          saveEEPROM();
        }
      if (modeSet == MODEEDITX || modeSet == MODEEDITZ)
      if (keyPad.getChar()=='C') 
        {                     //0-mm 1-inch
          setupSet.mm_inch = !setupSet.mm_inch;
          saveEEPROM();      
        }
      
    }
    if (now - lastTimeKeyPressed >= KEYPAD_1_PRESS_TIME && isUnpressed)
    {
      char ch = keyPad.getChar();     // note we want the translated char
      if (ch != 'N' || ch != 'F')
      if (ch=='A' || ch=='B')
      {
        Serial.print(ch);//    Serial.print(" \t");    //TAB
        lastTimeKeyPressed = now;
        
        if (lastCodeKeyPressed[0] == ch)
        {
          if (now - isPressedTimeOut <= KEYPAD_2ST_PRESS_TIMEOOUT)
          {   // 2ST press              // RESER TARE TO 0
            keypad_press_count++;
            isPressedTimeOut = now;
            lastTimeKeyPressed = now;
            if (keypad_press_count >=1)
            {
              keypad_press_count = 0;
              lastCodeKeyPressed[0] = 255;
            }
            if (ch=='A')
            if (modeSet == MODEEDITX)
            {
              setupSet.tareX[setupSet.memoryChX] = rawValueX;
              inputString = "";
              inputValueX = 0;
              inputValueZ = 0;
              modeSet = MODEWORK;
              tone(BEEP_PIN, 600,200);
            }
            else tone(BEEP_PIN, 1000,100);
            if (ch=='B')
            if (modeSet == MODEEDITZ)
            {
              setupSet.tareZ[setupSet.memoryChZ] = rawValueZ;
              inputString = "";
              inputValueX = 0;
              inputValueZ = 0;
              modeSet = MODEWORK;
              tone(BEEP_PIN, 600,200);
            }
            else tone(BEEP_PIN, 1000,100);
            
            Serial.println(" _|_|_ ");
            saveEEPROM();
          }else
          {                               //EXIT Edit mode
            lastTimeKeyPressed = now;
            isPressedTimeOut = now;
            keypad_press_count = 0;
            lastCodeKeyPressed[0] = 255;

            if (ch=='A')
            if (modeSet == MODEEDITX)
            {
              modeSet = MODEWORK; 
              setupSet.memoryChX = memoryCh;
            }
            if (ch=='B')
            if (modeSet == MODEEDITZ)
            {
              modeSet = MODEWORK; 
              setupSet.memoryChZ = memoryCh;
            }
            tone(BEEP_PIN, 1500,100);
            Serial.println(" _-_ ");
          }
        } else 
        {   // 1ST press                // ENTER Edit mode
          lastCodeKeyPressed[0] = ch;
          keypad_press_count = 0;
          isPressedTimeOut = now;
          lastTimeKeyPressed = now;

          if (ch=='A')
          if (modeSet == MODEWORK)
            {
              modeSet = MODEEDITX; 
              memoryCh = setupSet.memoryChX;
              tone(BEEP_PIN, 1000,100);
            }
          else
            if (modeSet == MODEEDITX)
            {
              modeSet = MODEWORK; 
              setupSet.memoryChX = memoryCh;
              lastCodeKeyPressed[0] = 255;
              tone(BEEP_PIN, 1500,100);
            }
          if (ch=='B')
          if (modeSet == MODEWORK)
            {
              modeSet = MODEEDITZ; 
              memoryCh = setupSet.memoryChZ;
              tone(BEEP_PIN, 1000,100);
            }
          else
            if (modeSet == MODEEDITZ)
            {
              modeSet = MODEWORK; 
              setupSet.memoryChZ = memoryCh;
              lastCodeKeyPressed[0] = 255;
              tone(BEEP_PIN, 1500,100);
            }

          Serial.println(" _|_ ");
        }
        isUnpressed = LOW;    
      }

      if (ch != 'N' || ch != 'F')
      if (ch=='C')
      {
        Serial.print(ch);//    Serial.print(" \t");    //TAB
        lastTimeKeyPressed = now;
        isPressedTimeOut = now;
        lastCodeKeyPressed[0] = ch;
        tone(BEEP_PIN, 1000,100);
        if (modeSet == MODEEDITX) {
          Serial.print(setupSet.memoryChX);
          Serial.println(" _|_ ");
          Serial.printf("ChX%i, ChZ%i, TareX=%.2f, TareZ=%.2f, X-%s Z-%s\n", setupSet.memoryChX+1, setupSet.memoryChZ+1, setupSet.tareX[setupSet.memoryChX], setupSet.tareZ[setupSet.memoryChZ], setupSet.plus_minus_X ? "direct" : "invert", setupSet.plus_minus_Z ? "direct" : "invert");
          setupSet.memoryChX++;
          if (setupSet.memoryChX >=6)
          {
            setupSet.memoryChX = 0;
          }          
        }
        if (modeSet == MODEEDITZ) {
          Serial.print(setupSet.memoryChZ);
          Serial.println(" _|_ ");
          Serial.printf("ChX%i, ChZ%i, TareX=%.2f, TareZ=%.2f, X-%s Z-%s\n", setupSet.memoryChX+1, setupSet.memoryChZ+1, setupSet.tareX[setupSet.memoryChX], setupSet.tareZ[setupSet.memoryChZ], setupSet.plus_minus_X ? "direct" : "invert", setupSet.plus_minus_Z ? "direct" : "invert");
          setupSet.memoryChZ++;
          if (setupSet.memoryChZ >=6)
          {
            setupSet.memoryChZ = 0;
          }
        }
        isUnpressed = LOW;
      }

      if (ch != 'N' || ch != 'F')
      if (ch == 'D' )
      {
        if (modeSet == MODEEDITX)
        if (inputString != "")
        {
          if ( Switch1State == HIGH ) {         //Switch Radius / Diametr
            if (setupSet.mm_inch) setupSet.tareX[setupSet.memoryChX] = rawValueX - (inputValueX * 25.4);
            else setupSet.tareX[setupSet.memoryChX] = rawValueX - inputValueX;
          }
          else
          {
            if (setupSet.mm_inch) inputValueX = inputValueX * 25.4 /2;
            else inputValueX = inputValueX /2;
            setupSet.tareX[setupSet.memoryChX] = rawValueX - inputValueX;
          }
          saveEEPROM();
        }
        if (modeSet == MODEEDITZ)
        if (inputString != "")
        {
          if (setupSet.mm_inch) setupSet.tareZ[setupSet.memoryChZ] = rawValueZ - (inputValueZ * 25.4);
          else setupSet.tareZ[setupSet.memoryChZ] = rawValueZ - inputValueZ;
          saveEEPROM();
        }
        modeSet = MODEWORK;
        inputString = "";
        inputValueX = 0;
        inputValueZ = 0;

        Serial.print(ch);//    Serial.print(" \t");    //TAB
        lastTimeKeyPressed = now;
        isPressedTimeOut = now;
        {   // 1ST press
          lastCodeKeyPressed[0] = ch;
          tone(BEEP_PIN, 1000,100);
        }
        isUnpressed = LOW;
      }
      
      if (ch != 'N' || ch != 'F')
      if (ch == '*' || ch == '#' || ch == '0' || ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5' || ch == '6' || ch == '7' || ch == '8' || ch == '9')
      {
        if (ch == '*' ) ch = '.';
        
        if (modeSet == MODEEDITX) 
        {
          if ('.' != ch && inputString == "0") {
            inputString = String(ch);
          } else if (ch) {
            inputString += String(ch);
          }          
          inputValueX = inputString.toFloat();//keypad_buffer = keypad_buffer *10 + (ch- '0');
        }
        if (modeSet == MODEEDITZ) 
        {
          if ('.' != ch && inputString == "0") {
            inputString = String(ch);
          } else if (ch) {
            inputString += String(ch);
          }          
          inputValueZ = inputString.toFloat();//keypad_buffer = keypad_buffer *10 + (ch- '0');
        }        
        Serial.print(ch);//    Serial.print(" \t");    //TAB
        lastTimeKeyPressed = now;
        isPressedTimeOut = now;
        lastCodeKeyPressed[0] = ch;
        tone(BEEP_PIN, 1000,100);
        isUnpressed = LOW;
      }
      
    }
  }

  if (modeSet == MODEEDITX){
    if (inputString == "")
    {
      if (setupSet.mm_inch) temporaryValueX = (rawValueX - setupSet.tareX[setupSet.memoryChX]) * 0.0393701;
      else temporaryValueX = rawValueX - setupSet.tareX[setupSet.memoryChX];
    }
    else
      temporaryValueX = inputValueX;
  }
  if (modeSet == MODEEDITZ){
    if (inputString == "")
    {
      if (setupSet.mm_inch) temporaryValueZ = (rawValueZ - setupSet.tareZ[setupSet.memoryChZ]) * 0.0393701;
      else temporaryValueZ = rawValueZ - setupSet.tareZ[setupSet.memoryChZ];
    }
    else
      temporaryValueZ = inputValueZ;
  }

  //SCALES
  readEncoder1();
  readEncoder2();

  if (probeWire){
    probeWire = LOW;
    tone(BEEP_PIN, 1500,50);
  }

static long tachoTimeout = 0;
//#define M_PI   3.14159265358979323846 /* pi */
  if (tachoProbe){
    tachoProbe = LOW;
    resultRPM_T = 60*1000*1000/tachoTime;
//    resultLine_T = 2*M_PI*resultValueX*600000/tachoTime;
    resultLine_T = 2*M_PI*resultValueX*resultRPM_T/1000;
    tachoTimeout = now;
  }
  if (now - tachoTimeout > MIN_RPM_DELAY){
    tachoProbe = LOW;
    resultRPM_T = 0;
    resultLine_T = 0;
  }

 if (modeSet == MODEWORK)
 if (now - perfomanceTimer > 1000)  
 {
    Serial.printf("x%.2f dx%.2f z%.2f %s\n", resultValueX, resultValueX*2, resultValueZ, setupSet.mm_inch ? "inch" : "mm");      
    Serial.printf("ChX%i, ChZ%i, TareX=%.2f, TareZ=%.2f, X-%s Z-%s\n", setupSet.memoryChX+1, setupSet.memoryChZ+1, setupSet.tareX[setupSet.memoryChX], setupSet.tareZ[setupSet.memoryChZ], setupSet.plus_minus_X ? "direct" : "invert", setupSet.plus_minus_Z ? "direct" : "invert");
    Serial.print("tachoTime - "); 
    Serial.print(tachoTime); 
    Serial.print(", RPM:"); 
    Serial.print(resultRPM_T); 
    Serial.print(", "); 
    Serial.print(resultLine_T);
    Serial.print(" m/min, "); 
    Serial.print(" FPS:");
    Serial.print(performanceDRO);
    performanceDRO = 0;
    perfomanceTimer = millis();
    Serial.println(" ");
 }

} 
// -- END loop() --


byte crc8(byte *buffer, byte size) {
  byte crc = 0;
  for (byte i = 0; i < size; i++) {
    byte data = buffer[i];
    for (int j = 8; j > 0; j--) {
      crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
      data >>= 1;
    }
  }
  return crc;
}

IRAM_ATTR void tachoIsr() {
static long timeLast = 0;
  tachoProbe = HIGH;
  tachoTime = micros() - timeLast;
  timeLast = micros();
}

IRAM_ATTR void probreIsr() {
  probeWire = HIGH;
  Serial.println("probe");
}

// -- END OF FILE --
