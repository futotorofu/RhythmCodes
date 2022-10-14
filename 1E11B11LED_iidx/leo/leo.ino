/* Arduino IIDX Controller Code for Leonardo
 * 1 Encoders + 11 Buttons + 11 HID controlable LED
 * with switchable analog/digital turntable output
 * release page
 * http://knuckleslee.blogspot.com/2018/06/RhythmCodes.html
 * 
 * Arduino Joystick Library
 * https://github.com/MHeironimus/ArduinoJoystickLibrary/
 * mon's Arduino-HID-Lighting
 * https://github.com/mon/Arduino-HID-Lighting
 */
#include <Joystick.h>
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD, 14, 0,
 true, true, false, false, false, false, false, false, false, false, false);

boolean hidMode, ttMode, state[1]={false}, set[2]={false};
int encTT=0, TTold=0;
unsigned long TTmillis;
const int GEAR = 144;   //number of gear teeth or ppr of encoder
const int TTdz = 0;     //digital tt deadzone (pulse)
const int TTdelay = 50;  //digital tt button release delay (millisecond)
byte EncPins[]    = {0,1};
byte SinglePins[] = {2,4,6,8,10,12,18,20,22,14,16};
byte ButtonPins[] = {3,5,7,9,11,13,19,21,23,15,17};
unsigned long ReactiveTimeoutMax = 1000;  //number of cycles before HID falls back to reactive

/* pin assignments
 *   TT Sensor to pin 0 and White to pin 1
 * current pin layout
 *  SinglePins {2,4,6,8,10,12,18,20,22,14,16} = LED 1 to 11
 *    connect pin to resistor and then + termnial of LED
 *    connect ground to - terminal of LED
 *  ButtonPins {3,5,7,9,11,13,19,21,23,15,17} = Button input 1 to 11
 *    connect button pin to ground to trigger button press
 *  Light mode detection by read first button while connecting usb 
 *   hold    = false = reactive lighting 
 *   release = true  = HID lighting with reactive fallback
 *  TT mode detection by read second button while connecting usb 
 *   hold    = false = digital turntable mode
 *   release = true  =  analog turntable mode
 */
 
byte ButtonCount = sizeof(ButtonPins) / sizeof(ButtonPins[0]);
byte SingleCount = sizeof(SinglePins) / sizeof(SinglePins[0]);
byte EncPinCount = sizeof(EncPins) / sizeof(EncPins[0]);
unsigned long ReactiveTimeoutCount = ReactiveTimeoutMax;

int ReportDelay = 700;
unsigned long ReportRate ;

void setup() {
  Serial.begin(9600) ;
  Joystick.begin(false);
  Joystick.setXAxisRange(-GEAR/2, GEAR/2-1);
  Joystick.setYAxisRange(-GEAR/2, GEAR/2-1);
  
  // setup I/O for pins
  for(int i=0;i<ButtonCount;i++) {
    pinMode(ButtonPins[i],INPUT_PULLUP);
  }
  for(int i=0;i<SingleCount;i++) {
    pinMode(SinglePins[i],OUTPUT);
  }
  for(int i=0;i<EncPinCount;i++) {
    pinMode(EncPins[i],INPUT_PULLUP);
  }

  //setup interrupts
  //with AC optical sensors you can pick up more inputs with a change interrupt
  attachInterrupt(digitalPinToInterrupt(EncPins[0]), doEncF0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EncPins[1]), doEncF1, CHANGE);
  
  // light and turntable mode detection
  hidMode = digitalRead(ButtonPins[0]);
   ttMode = digitalRead(ButtonPins[1]);
  while(digitalRead(ButtonPins[0])==LOW
       |digitalRead(ButtonPins[1])==LOW) {
    if ( (millis() % 1000) < 500) {
      for(int i=0;i<SingleCount;i++) {
        digitalWrite(SinglePins[i],HIGH);
      }
    }
    else if ( (millis() % 1000) > 500) {
      for(int i=0;i<SingleCount;i++) {
        digitalWrite(SinglePins[i],LOW);
      }
    }
  }
  for(int i=0;i<SingleCount;i++) {
    digitalWrite(SinglePins[i],LOW);
  }

  //boot light
  for(int i=0; i<ButtonCount; i++) {
    digitalWrite(SinglePins[i],HIGH);
    delay(80);
    digitalWrite(SinglePins[i],LOW);
  }
  for(int i=ButtonCount-2; i>=0; i--) {
    digitalWrite(SinglePins[i],HIGH);
    delay(80);
    digitalWrite(SinglePins[i],LOW);
  }
  for(int i=0;i<ButtonCount ;i++) {
    digitalWrite(SinglePins[i],HIGH);
  }
    delay(500);
  for(int i=0;i<ButtonCount ;i++) {
    digitalWrite(SinglePins[i],LOW);
  }
  
  //initialize encoder state so we can judge rise and fall
  set[0] = digitalRead(EncPins[0]);
  set[1] = digitalRead(EncPins[1]);
  
} //end setup

void loop() {
  ReportRate = micros() ;

  //read buttons
  for(int i=0;i<ButtonCount;i++) {
    Joystick.setButton (i,!(digitalRead(ButtonPins[i])));
  }

  if(hidMode==false || (hidMode==true && ReactiveTimeoutCount>=ReactiveTimeoutMax)){
    for(int i=0;i<ButtonCount;i++) {
      digitalWrite (SinglePins[i],!(digitalRead(ButtonPins[i])));
    }
  }
  else if(hidMode==true) {
    ReactiveTimeoutCount++;
  }

  //read encoders
  //analog mode, detect overflow and rollover
  if(ttMode==true) {
    if(encTT < -128 || encTT > 127) {
		encTT = constrain (encTT*-1, -128, 127);
	}
    Joystick.setXAxis(encTT);
  }
  //digital mode
  else if(ttMode==false) {
    if(encTT != TTold) {
      if(encTT < -TTdz) {
        Joystick.setButton (11,1);
        Joystick.setButton (12,0);
        TTmillis = millis();
        encTT = 0;
      }
      else if(encTT > TTdz) {
        Joystick.setButton (11,0);
        Joystick.setButton (12,1);
        TTmillis = millis();
        encTT = 0;
      }
    }
    if((millis() - TTmillis) > TTdelay) {
      Joystick.setButton (11,0);
      Joystick.setButton (12,0);
      TTmillis = millis();
      encTT = 0;
    }
    TTold = encTT;
  }

  //report
  Joystick.sendState();
  delayMicroseconds(ReportDelay);
  //ReportRate Display
  //Serial.print(micros() - ReportRate) ;
  //Serial.println(" micro sec per loop") ;
}//end loop

//Interrupts
void doEncF0() {
	int prevState = set[0];
	set[0] = digitalRead(EncPins[0]);
	
	if(prevState == 0 && set[0] == 1) { //Encoder 0 Rise
		if(digitalRead(EncPins[1]) == LOW) { 
			encTT++;
		} else {
			encTT--;
		}
		//debugTT(0, true);
	} else if (prevState == 1 && set[0] == 0) { //Encoder 0 Fall
		if(digitalRead(EncPins[1]) == HIGH) { 
			encTT++;
		} else {
			encTT--;
		}
		//debugTT(0, false);
	}
}


void doEncF1() {
	int prevState = set[1];
	set[1] = digitalRead(EncPins[1]);
	if(prevState == 0 && set[1] == 1) { //Encoder 1 Fall
		if(digitalRead(EncPins[0]) == HIGH) { 
			encTT++;
		} else {
			encTT--;
		}
		//debugTT(1, false);
	} else if (prevState == 1 && set[1] == 0) { //Encoder 0 Rise
		if(digitalRead(EncPins[0]) == LOW) { 
			encTT++;
		} else {
			encTT--;
		}
		//debugTT(1, true);
	}
}

void debugTT(int enc, int rise) {
	Serial.print(encTT);
	if(enc) {
		Serial.print(" Encoder 1 " );
	} else { 
		Serial.print(" Encoder 0 ");
	}
	if(rise) {
		Serial.println(" RISE");
	} else {
		Serial.println(" FALL");
	}
}
