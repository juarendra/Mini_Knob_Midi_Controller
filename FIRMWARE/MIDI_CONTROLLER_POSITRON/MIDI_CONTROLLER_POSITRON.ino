#include <USBComposite.h>
USBMIDI midi;
//#include <ResponsiveAnalogRead.h> 
#include <FastLED.h>

#define LED_PIN     PB10        // Choose a tested pin
#define NUM_LEDS    5
#define BRIGHTNESS  100
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

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
int potMax = 3070;

bool debug_pot = false;
byte midiCh = 0;
byte cc = 1;   
byte NUM_BANK = 0;

const int NUM_SAMPLES = 10;

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

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
  // Light up LEDs based on active bank
  for(int i = 0; i < NUM_LEDS; i++) {
    if (i == NUM_BANK) {
      leds[i] = CRGB::White;   // Active bank LED to white
    } else {
      leds[i] = CRGB::Black;   // Others off
    }
  }
  FastLED.show();

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

void sendMidiValue() {
  int potMin = 10;    
  int potMax = 3070;

  for (int i = 0; i < NUM_POT; i++) {
    int total = 0;

    // Take multiple samples to average
    for (int s = 0; s < NUM_SAMPLES; s++) {
      total += analogRead(analogInputs[i]);
      delayMicroseconds(100); // Small delay between reads
    }

    reading_pot = total / NUM_SAMPLES; // Calculate average reading
    potCState[i] = reading_pot;

    // Map and constrain stable average value
    midiCState[i] = map(potCState[i], potMin, potMax, 127, 0);
    midiCState[i] = constrain(midiCState[i], 0, 127);

    // Dead zones to further stabilize extremes
    if (midiCState[i] <= 1) midiCState[i] = 0;
    if (midiCState[i] >= 126) midiCState[i] = 127;

    potVar = abs(potCState[i] - potPState[i]);
    if (potVar > varThreshold) {
      PTime[i] = millis();
    }
    timer[i] = millis() - PTime[i];

    potMoving = timer[i] < TIMEOUT;

    // Only send MIDI if there is meaningful change
    if (abs(midiCState[i] - midiPState[i]) > 1) {
      midi.sendControlChange(midiCh, cc + i + (NUM_BANK * 10), midiCState[i]);
      potPState[i] = potCState[i];
      midiPState[i] = midiCState[i];
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