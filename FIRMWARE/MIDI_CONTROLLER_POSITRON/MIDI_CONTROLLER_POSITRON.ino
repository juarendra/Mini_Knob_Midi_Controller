#include <USBComposite.h>
USBMIDI midi;
//#include <ResponsiveAnalogRead.h> 

const int NUM_POT = 10;
const int NUM_BUT = 5;
const int analogInputs[NUM_POT] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PB0, PB1};
const int digitalInput[NUM_BUT] = {PB9, PB8, PB7, PB6, PB5};
int analogSliderValues[NUM_POT];
//ResponsiveAnalogRead responsivePot[NUM_POT] = {}; 
int potCState[NUM_POT] = { 0 };  // Current state of the pot
int potPState[NUM_POT] = { 0 };  // Previous state of the pot
int potVar = 0;                 // Difference between the current and previous state of the pot

int midiCState[NUM_POT] = { 0 };  // Current state of the midi value
int midiPState[NUM_POT] = { 0 };  // Previous state of the midi value
int reading_pot = 0;

const int TIMEOUT = 300;              // Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 20;          // Threshold for the potentiometer signal variation
boolean potMoving = true;             // If the potentiometer is moving
unsigned long PTime[NUM_POT] = { 0 };  // Previously stored time
unsigned long timer[NUM_POT] = { 0 };  // Stores the time that has elapsed since the timer was reset

int potMin = 0;
int potMax = 4096;

bool debug_pot = false;
byte midiCh = 0;
byte cc = 1;   
byte NUM_BANK = 0;


void setup() {
  for (int i = 0; i < NUM_POT; i++) {
    pinMode(analogInputs[i], INPUT);
  }
  for (int i = 0; i < NUM_BUT; i++) {
    pinMode(digitalInput[i], INPUT_PULLUP);
  }
  if (debug_pot){
    Serial.begin(9600);
  }
  else{
    USBComposite.setProductId(0x0031);
    midi.begin();
    while (!USBComposite);
  }
}

void loop() {
  for (int i = 0; i < NUM_BUT; i++){
    if (!digitalRead(digitalInput[i])){
      NUM_BANK = i;
    }

  }
  if(debug_pot){
    updateSliderValues();
    printSliderValues();
    delay(100);
  }
  else {
    sendMidiValue();
    delay(10);
  }
}

void sendMidiValue(){
  for (int i = 0; i < NUM_POT; i++) {
    reading_pot = analogRead(analogInputs[i]);
    potCState[i] = reading_pot;

    midiCState[i] = map(potCState[i], potMin, potMax, 0, 127); 
    if (midiCState[i] < 0) {
      midiCState[i] = 0;
    }
    if (midiCState[i] > 127) {
      midiCState[i] = 0;
    }
    potVar = abs(potCState[i] - potPState[i]);
    if (potVar > varThreshold) {  // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis();        // Stores the previous time
    }
    timer[i] = millis() - PTime[i];  // Resets the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) {  // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
      potMoving = true;
    } else {
      potMoving = false;
    }

    if (potMoving == true) {  // If the potentiometer is still moving, send the change control
      if (midiPState[i] != midiCState[i]) {

        //use if using with ATmega32U4 (micro, pro micro, leonardo...)
        midi.sendControlChange(midiCh, cc + i + (NUM_BANK*10), midiCState[i]);  //  (channel, CC number,  CC value)

        potPState[i] = potCState[i];  // Stores the current reading of the potentiometer to compare with the next
        midiPState[i] = midiCState[i];
      }
    }
  }
}

void updateSliderValues() {
  for (int i = 0; i < NUM_POT; i++) {
     analogSliderValues[i] = analogRead(analogInputs[i]);
  }
}

void printSliderValues() {
  for (int i = 0; i < NUM_POT; i++) {
    String printedString = String("Slider #") + String(i + 1) + String(": ") + String(analogSliderValues[i]) + String(" mV");
    Serial.write(printedString.c_str());

    if (i < NUM_POT - 1) {
      Serial.write(" | ");
    } else {
      Serial.write("\n");
    }
  }
}