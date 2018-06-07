/* ********* DDS 9850 VFO  *********

This is the Arduino program to control the evaluation board for an AD9850 chip.
Purpose: The unit is being used as a VFO for Ham radio vintage equipment.
Why?  It is difficult to find vintage stable VFOs, especially those that have a frequency offset.

This has two modes:
1 - Ham band VFO, 20 to 160 meters for use in vintage equipment
2 - Ham band VFO for Central Electronics 20A exciter (frequencies are different than resultant frequencies)

Last update 6/29/17 - coalesce frequency change activities into one function, misc cleanups pre-blog

Use: encoder to select frequency, 20 meters, 14.000 mHz defaults.  Press encloder to cycle through band changes.
************************************ */

#include <SPI.h> // for AD9850
#include <Wire.h>  // Comes with Arduino IDE
#include <LiquidCrystal_I2C.h>

// AD 9850 board
#define W_CLK 8       // Pin 8 - connect to AD9850 module word load clock pin (CLK)
#define FQ_UD 9       // Pin 9 - connect to freq update pin (FQ)
#define DAT 10       // Pin 10 - connect to serial data load pin (DATA)
#define RESET 11      // Pin 11 - connect to reset pin (RST). 
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

// set the LCD address to 0x27 for a 16 chars 2 line display A FEW use address 0x3F, 20x4
// Set the pins on the I2C chip used for LCD connections: addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

//Rotary Encoder Init
static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
static int pinB = 3; // Our second hardware interrupt pin is digital pin 3
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile int encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile int oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent

// AD9850 frequency calculation
double fAD9850Val = 0;

// Frequency Determination and Display
int iFreqChangeFactor = 0 ;// Can be -1, 0, 1 corresponding to CCW, None, CW rotation respectively
float fFreqChangeAmount = 0.001 ;  //1 kHz
float fFrequency; //  mHz
float fDDSFrequency; // used when display and DDS frequencies are different; i.e. in case of CE 20a exciter
int iBandMeters;
float fFreqMax;
float fFreqMin;
char sBandMeters[3];
char sFrequency[6];
char sDDSFrequency[6]; 

// Mode 1=VFO, 2=VFO for Central Electronics 20A Injection Frequency masking (uses 9mc oscillator)
int iMode = 1; 
// set up the momentary switches
// Band Button
const int iBandPin = 4;
// Mode Button
const int iModePin = 5;

void setup()  
{
 // configure arduino data pins for output
 pinMode(FQ_UD, OUTPUT);
 pinMode(W_CLK, OUTPUT);
 pinMode(DAT, OUTPUT);
 pinMode(RESET, OUTPUT);
   
 pulseHigh(RESET);
 pulseHigh(W_CLK);
 pulseHigh(FQ_UD);  // this pulse enables serial mode - Datasheet page 12 figure 10
 
 // initialize Buttons
 pinMode(iBandPin, INPUT);
 pinMode(iModePin, INPUT);
  
 lcd.begin(20,4);   // initialize the lcd for 20x4; turn on backlight
 delay(100); //some setup delay time 
 
  // Splash: 3 blinks of backlight 
 Splash();
 
  //Rotary Encoder
 pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
 pinMode(pinB, INPUT_PULLUP); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
 attachInterrupt(0,PinA,RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
 attachInterrupt(1,PinB,RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)
 //Serial.begin(9600); // start the serial monitor link
  
  //Set the default frequency on Mode 1, and start DDS
  ChangeBand();
  SetFreq(fFrequency);
}

void loop()   
{
iFreqChangeFactor = 0 ;
// Is the Rotary Encoder turned?  If so, increment the change factor
if(oldEncPos != encoderPos) 
{
  if (encoderPos > oldEncPos)
  { iFreqChangeFactor = 1; }
  else
  { iFreqChangeFactor = -1; }
  oldEncPos = encoderPos;

  // Calculate the new VFO display frequency
   CalcVFOFrequency() ;
   SetFreq(fFrequency);
   delay(50);
}

// Change the band if the button is pressed
int valBand = digitalRead(iBandPin);
if (valBand == HIGH)
  { 
  ChangeBand(); 
  SetFreq(fFrequency);
  delay(500);
  }
  
// Change the mode if the button is pressed
int valMode = digitalRead(iModePin);
if (valMode == HIGH)
  { 
  // if the mode is changing, reset iBandMeters
  iBandMeters = 99;
  if (iMode == 1)
     { iMode = 2; }
  else
     { iMode = 1; }     
  ChangeBand(); 
  SetFreq(fFrequency);
  delay(500);
  }
  
} 
// end loop

void ChangeBand()
{
  switch (iBandMeters) {
    case 10:
      fFrequency = 21.000;
      fFreqMax = 21.450;
      iBandMeters = 15;
      break;
    case 15:
      fFrequency = 14.000;
      fFreqMax = 14.350;
      iBandMeters = 20;
      break;
    case 20:
      fFrequency = 7.000;
      fFreqMax = 7.300;
      iBandMeters = 40;
      break;
    case 40:
      fFrequency = 3.500;
      fFreqMax = 4.000;
      iBandMeters = 80;
      break;
    case 80:
      fFrequency = 1.800;
      fFreqMax = 2.000;
      iBandMeters = 160;
      break;
    case 160:
      fFrequency = 28.000;
      fFreqMax = 29.700;
      iBandMeters = 10;
      break;
    default:
      fFrequency = 14.000;
      fFreqMax = 14.350;
      iBandMeters = 20;
  }
  fFreqMin = fFrequency;
}

void CalcVFOFrequency()
{
    // Increment or decrement the frequency by the change factor depending on rotation
    fFrequency = fFrequency + (fFreqChangeAmount * iFreqChangeFactor); 

    // if we're hitting the band maximum or minimum reset to opposite side of the band
    if (fFrequency < fFreqMin )
       { fFrequency = fFreqMax; } 
    else if (fFrequency > fFreqMax)
       { fFrequency = fFreqMin; } 
    else {}
}

// in the case of Mode 2, CE 20a Exciter, set the DDS offset by 9 mHz, so the exciter results in the display frequency
// many mods to this section 10/02/17 decided to add 9 mHz to display frequency, and expect exciter to heterodyne the difference
// between the VFO and the 9 mHz offset - should generate the display frequency, testing TBD. 
float CalcCE20aFrequency(float fCE20aFreq)
{
  switch (iBandMeters) {
  case 10:
    fCE20aFreq = fCE20aFreq + 9; // +9, 
    break;
  case 15:
    fCE20aFreq = fCE20aFreq + 9.000; // +9
    break;
  case 20:
    fCE20aFreq = fCE20aFreq + 9.000; // -9
    break;
  case 40:
    fCE20aFreq = fCE20aFreq + 9.000; // +9
    break;
  case 80:
    fCE20aFreq = fCE20aFreq + 9; 
    break;
  case 160:
    fCE20aFreq = fCE20aFreq + 9; //  e.g. 2.0 passed vs 11.0 processed
   break;
  default:  //20 meters
    fCE20aFreq = fCE20aFreq + 9.000; 
  }    
  return fCE20aFreq;
  }

//This function sets the DDS output to the desired frequency and provides LCD feedback.
void SetFreq(float fFreq)
{
  // Set the DDS RF Frequency; first check to see if there's an offset for the CE 20a functionality
  fDDSFrequency = fFreq ;
  if (iMode == 2)  // CE 20a exciter
   { fDDSFrequency = CalcCE20aFrequency(fFreq); }
  
  fAD9850Val = (float) (fDDSFrequency*1000000);
  int32_t freq = fAD9850Val * 4294967295/125000000;  // note 125 MHz clock on 9850
  for (int b=0; b<4; b++, freq>>=8) {
      tfr_byte(freq & 0xFF);
  }
  tfr_byte(0x000);   // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
  
  // User display update
  // Band Title    
  dtostrf(iBandMeters, 3, 0, sBandMeters);
  lcd.setCursor(1,1);
  lcd.print(sBandMeters);
  lcd.setCursor(4, 1);
  lcd.print(" meters");

  // Frequency
  if (iMode == 1)
   { 
    lcd.setCursor(4,0);
    lcd.print("  HAM VFO  ");
    lcd.setCursor(2, 2);
    lcd.print("FOut"); 
    lcd.setCursor(7, 2);
    dtostrf(fFreq, 6, 3, sFrequency);
    lcd.print(sFrequency);
    // overwrite characters leftover from Mode 2
    lcd.setCursor(2, 3);
    lcd.print("           "); 
   }
  else
   {
    lcd.setCursor(5,0);
    lcd.print("CE 20a VFO");
    lcd.setCursor(2, 2);
    lcd.print("Dial");       
    lcd.setCursor(7, 2);
    dtostrf(fFreq, 6, 3, sFrequency);
    lcd.print(sFrequency);
    // second line
    lcd.setCursor(2, 3);
    lcd.print("FOut"); 
    dtostrf(fDDSFrequency, 6, 3, sDDSFrequency);
    lcd.setCursor(7, 3);
    lcd.print(sDDSFrequency);
     }
}

void Splash()
{
  for(int i = 0; i< 3; i++)
  {
    lcd.backlight();
    delay(300);
    lcd.noBacklight();
    delay(300);
  }
  lcd.backlight(); 
}

void PinA(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; // read all eight pin values then strip away all but pinA and pinB's values
  if(reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00000100) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  sei(); //restart interrupts
}

void PinB(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; //read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos ++; //increment the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00001000) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
  sei(); //restart interrupts
}

// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void tfr_byte(byte data)
{
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DAT, data & 0x01);
    pulseHigh(W_CLK);   //after each bit sent, CLK is pulsed high
  }
}
