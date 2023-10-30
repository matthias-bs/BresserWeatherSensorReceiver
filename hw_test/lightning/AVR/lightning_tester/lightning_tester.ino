// Arduino sketch for AS3935 lightning generator
//
// Configures Timer 2 to produce 500kHz output
// Turns on and off power to output transistor to produce RF pattern
// Flashes LED and beeps for indication
//
// by Michael Gasperi
//
// 20231030 M.Prinke: Added config for ATtiny841

#include <Arduino.h>

#if defined(ARDUINO_AVR_ATTINY841)
  #define PIN_LED LED_BUILTIN
  #define PIN_PWR 6
  #define PIN_OSC 2
#else
  #define PIN_LED 13
  #define PIN_PWR 12
  #define PIN_OSC 11
#endif

void setup() {
  pinMode(PIN_OSC,OUTPUT); //Timer 2 Output A
  pinMode(PIN_PWR,OUTPUT); //Output to power RF
  pinMode(PIN_LED,OUTPUT); //Output for LED and beeper

#if defined(ARDUINO_AVR_ATTINY841)
  // See https://www.mikrocontroller.net/topic/400521#4625494:
  //
  // Timer/Counter 2 initialization
  // Clock source: System Clock
  // Clock value: 8 MHz
  // Mode: CTC top=OCR2A
  // OC1A output: Toggle on compare match
  // OC1B output: Disconnected
  // Noise Canceler: Off
  // Input Capture on Falling Edge
  // Timer Period: 125 ns
  // Output Pulse(s):
  // OC2A Period: 10 ms Width: 2 us / 1 us
  // Timer1 Overflow Interrupt: Off
  // Input Capture Interrupt: Off
  // Compare A Match Interrupt: Off
  // Compare B Match Interrupt: Off
  TCCR2A=(0<<COM2A1) | (1<<COM2A0) | (0<<COM2B1) | (0<<COM2B0) | (0<<WGM21) | (0<<WGM20);
  TCCR2B=(0<<ICNC2) | (0<<ICES2) | (0<<WGM23) | (1<<WGM22) | (0<<CS22) |  (0<<CS21) | (1<<CS20);
  TCNT2H=0x00;
  TCNT2L=0x00;
  ICR2H=0x00;
  ICR2L=0x00;
  OCR2AH=0x00;
  OCR2AL=0x07; // System clock 8 MHz - divide by 16
  OCR2BH=0x00;
  OCR2BL=0x00;
  
  // Timer/Counter Compare Outputs signals routed to TOCCn pins:
  // Nothing -> TOCC0
  // OC2A -> TOCC1
  // Nothing -> TOCC2
  // Nothing -> TOCC3
  // Nothing -> TOCC4
  // Nothing -> TOCC5
  // Nothing -> TOCC6
  // Nothing -> TOCC7
  TOCPMSA0=(0<<TOCC3S1) | (0<<TOCC3S0) | (0<<TOCC2S1) | (0<<TOCC2S0) | 
           (1<<TOCC1S1) | (0<<TOCC1S0) | (0<<TOCC0S1) | (0<<TOCC0S0);
  TOCPMSA1=(0<<TOCC7S1) | (0<<TOCC7S0) | (0<<TOCC6S1) | (0<<TOCC6S0) | 
           (0<<TOCC5S1) | (0<<TOCC5S0) | (0<<TOCC4S1) | (0<<TOCC4S0);
  TOCPMCOE=(0<<TOCC7OE) | (0<<TOCC6OE) | (0<<TOCC5OE) | (0<<TOCC4OE) | 
           (0<<TOCC3OE) | (0<<TOCC2OE) | (1<<TOCC1OE) | (0<<TOCC0OE);

#else
  TCCR2A = 0x42; //Timer 2 control register A = toggle output A on compare
  TCCR2B = 0x01; //Timer 2 control register B = clock no prescale = 16MHz

  OCR2A = 15;    //Timer 2 compare register A = will divide by 32
  TCNT2 = 0;
#endif

}

void loop() { 

  // make 1, 2, 3, and 4 lighning spike patterns

  for (int j=1;j<5;j++){
    // flash led and beep the numer of patterns
    for (int i=0;i<j;i++){
      digitalWrite(PIN_LED,HIGH);
      delay(25); //short blink or beep
      digitalWrite(PIN_LED,LOW);
      delay(50);
    }

    // generate the number of spike patterns
    for (int i=0;i<j;i++){
      pinMode(PIN_OSC,OUTPUT);    //Turn on 500kHz output
      digitalWrite(PIN_PWR,HIGH); //Turn on power to RF 
      delayMicroseconds(25); //Wait to charge RF capacitor up
      digitalWrite(PIN_PWR,LOW);  //Turn off power to RF  
      delayMicroseconds(600);//Wait 600uS for decay
      pinMode(PIN_OSC,INPUT);     //Turn off 500kHz output by making it input
    }
    delay(5000); //Wait 5 seconds for chip to react
  } 
}
