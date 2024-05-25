// Arduino Uno program to control an array of 256 bright LEDs for optogenetics experiments with Arabidopsis plants
//Opto_Plant_LEDS_V2
// Author: S.D. Tyerman
// Version: 2.0 Now with better control of Led brightness for red and blue and user input via pots for timing.
// Date: 25.05.2024

#include <FastLED.h>

// Pin definitions
#define LED_PIN     3   // Controls the LED array
#define BLUE_RED    4   // To change color
#define BLINK_PIN   5   // Set high for 1-second blue pulse at desired intensity
#define LED_BLACK   6   // If low and BLINK_PIN high, set LEDs off to give just blue flash
#define NUM_LEDS    256 // Currently 4 x 8x8 GLOWbit Arrays from Core hooked together to give 256 LEDs
#define BUTTON_PIN  2   // Button pin (interrupt-enabled)
#define SIGNAL_PIN  7   // Signal pin to indicate blue pulse happening

// Constants
const uint16_t MAX_PULSE_TIME = 10000;   // Maximum pulse time in milliseconds (e.g., 5 seconds)
const uint16_t MAX_BETWEEN_TIME = 60000; // Maximum time between pulses in milliseconds (e.g., 1 minute)
uint32_t PULSE_TIME = 1000;     // 1-second pulse duration
uint32_t BETWEEN_TIME = 1000;  // 10-second delay between pulses in blink mode

// LED array
CRGB leds[NUM_LEDS];

// Global variables
volatile bool buttonPressed = false;
CRGB evenColor, oddColor;
static uint32_t lastPulseTime = 0;

int safeAnalogRead(int pin) {
  int reading = analogRead(pin); // First reading
  delay(1); // Short delay for ADC to stabilize
  reading = analogRead(pin); // Second reading
  return reading;
}

void setup() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BLINK_PIN, INPUT_PULLUP);   // Attach toggle switch here
    pinMode(LED_BLACK, INPUT_PULLUP);
    pinMode(BLUE_RED, INPUT_PULLUP);
    pinMode(SIGNAL_PIN, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);
}

void loop() {
    uint16_t blueIntens = safeAnalogRead(A0);  // Read 10-turn linear potentiometer value for intensity of blue LED
    uint16_t redIntens = safeAnalogRead(A1);   // Read 10-turn linear potentiometer value for intensity of red LED
    uint8_t blueVal = map(blueIntens, 0, 1023, 0, 255); // Map input to allowed range
    uint8_t redVal = map(redIntens, 0, 1023, 0, 255); // Map input to allowed range

  

    // Read user input for PULSE_TIME
    uint16_t pulseTimeInput = safeAnalogRead(A2); // Read from analog input A2
    PULSE_TIME = map(pulseTimeInput, 0, 1023, 50, MAX_PULSE_TIME); // Map input to allowed range

    // Read user input for BETWEEN_TIME
    uint16_t betweenTimeInput = safeAnalogRead(A3); // Read from analog input A3
    BETWEEN_TIME = map(betweenTimeInput, 0, 1023, 200, MAX_BETWEEN_TIME); // Map input to allowed range

    if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == HIGH) {handleBlinkMode(blueVal, redVal);}
    if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == LOW) {handleContinuousMode(blueVal, redVal);}
    if (digitalRead(BLINK_PIN) == LOW && digitalRead(LED_BLACK) == HIGH) {handleBlackMode();}
    if (buttonPressed) {handleButtonPress(blueVal, redVal);}

    //delay(10);  // Delay to allow for other tasks
}

//## Handle blink mode with red and blue pulses
void handleBlinkMode(uint8_t blueVal, uint8_t redVal) {
    static CRGB evenColor, oddColor;
    static unsigned long lastBlinkTime = 0;
    static bool isPulsing = false;
    unsigned long currentMillis = millis();

    if (currentMillis - lastBlinkTime >= (isPulsing ? PULSE_TIME : BETWEEN_TIME)) {
        if (isPulsing) {
            evenColor = CRGB(redVal, 0, 0);
            oddColor = CRGB(0, 0, 0);
            digitalWrite(SIGNAL_PIN, LOW);
        } else {
            evenColor = CRGB(redVal, 0, 0);
            oddColor = CRGB(0, 0, blueVal);
            digitalWrite(SIGNAL_PIN, HIGH);
        }
        fillLEDMatrix(evenColor, oddColor);
        lastBlinkTime = currentMillis;
        isPulsing = !isPulsing;
    }
}
//## Handle continuous mode with red or blue LEDs
void handleContinuousMode(uint8_t blueVal, uint8_t redVal) {
    static CRGB evenColor, oddColor;

    
        if (digitalRead(BLUE_RED) == HIGH) {
            evenColor = CRGB(redVal, 0, 0);
            oddColor = CRGB(0, 0, 0);
        } else {
            evenColor = CRGB(0, 0, 0);
            oddColor = CRGB(0, 0, blueVal);
        }
        fillLEDMatrix(evenColor, oddColor);
  
}

//## Handle continuous black mode with blue pulse on button press
void handleBlackMode() {
    static CRGB evenColor, oddColor;

    
        evenColor = CRGB(0, 0, 0);
        oddColor = CRGB(0, 0, 0);
        fillLEDMatrix(evenColor, oddColor);
    
}

//## Handle button press for blue pulse
void handleButtonPress(uint8_t blueVal, uint8_t redVal) {
    static CRGB evenColor, oddColor;

    
        if (digitalRead(BLINK_PIN) == LOW && digitalRead(LED_BLACK) == HIGH) {
            evenColor = CRGB(0, 0, 0);
            oddColor = CRGB(0, 0, blueVal);
        } else if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == LOW) {
            evenColor = CRGB(redVal, 0, 0);
            oddColor = CRGB(0, 0, blueVal);
        }
        fillLEDMatrix(evenColor, oddColor);
        digitalWrite(SIGNAL_PIN, HIGH);  // Indicate blue pulse
        delay(PULSE_TIME);
        digitalWrite(SIGNAL_PIN, LOW);   // Indicate pulse ended
        if (digitalRead(BLINK_PIN) == LOW && digitalRead(LED_BLACK) == HIGH) {
            evenColor = CRGB(0, 0, 0);
            oddColor = CRGB(0, 0, 0);
        } else if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == LOW) {
            evenColor = CRGB(redVal, 0, 0);
            oddColor = CRGB(0, 0, 0);
        }
        fillLEDMatrix(evenColor, oddColor);
        buttonPressed = false;
    
}

//## Fill the LED matrix with desired colors
void fillLEDMatrix(CRGB color1, CRGB color2) {
    FastLED.clear();  // Clear the LED array
    for (int i = 0; i < NUM_LEDS; i += 3) {
        leds[i] = color1;
        if (i + 1 < NUM_LEDS) {
            leds[i + 1] = color2;
            leds[i + 2] = color2;
        }
    }
    FastLED.show();
}

// Interrupt Service Routine (ISR) for button press
void buttonISR() {
    buttonPressed = true;
}