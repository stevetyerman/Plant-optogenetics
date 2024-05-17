// Arduino Uno program to control an array of 256 bright LEDs for optogenetics experiments with Arabidopsis plants
//relatively random pattern of Red and Blue with more Blue leds activated when required than Red.  See FillLEDMatrix function
//Current settings give 80 umol m-2 s-1 of red at 5 cm distance (full power), 304 umol m-2 s-1 of blue at 5cm distance, note 2x number of leds on for blue compared to red in current setting,  can also change to 50:50 if desired using parameter fro loops in FillLEDMatrix function. 
// Author: S.D. Tyerman
// Version: 1.0
// Date: 24.04.2024

#include <FastLED.h>

// Pin definitions
#define LED_PIN     3   // Controls the LED array
#define BLUE_RED    4   // to change color
#define BLINK_PIN   5   // Set high for 1-second blue pulse at desired intensity
#define LED_BLACK   6   // If low and BLINK_PIN high, set LEDs off to give just blue flash
#define NUM_LEDS    256 // Currently 4 x 8x8 GLOWbit Arrays from Core hooked together to give 256 LEDs
#define BUTTON_PIN  2   // Button pin (interrupt-enabled)
#define SIGNAL_PIN  7   // Signal pin to indicate blue pulse happening

// Constants, pulse times are currently set, new version will have variable pulse times set using another two 10 turn linear potentiameters
const int PULSE_TIME = 1000;     // 1-second pulse duration
const int BETWEEN_TIME = 10000;  // 10-second delay between pulses in blink mode

// LED colors
CRGB evenColor = CRGB::Blue;
CRGB oddColor = CRGB::Red;

// LED array
CRGB leds[NUM_LEDS];

// Global variables
volatile bool buttonPressed = false;

void setup() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(10);  // Initial brightness

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BLINK_PIN, INPUT_PULLUP);   // Attach toggle switch here
    pinMode(LED_BLACK, INPUT_PULLUP);
    pinMode(BLUE_RED, INPUT_PULLUP);
    pinMode(SIGNAL_PIN, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
}

void loop() {
    int blueIntens = analogRead(A0);  // Read 10 tuen linear potentiometer value for intensity of blue LED
    int redIntens = analogRead(A1);   // Read 10 turn linear potentiometer value for intensity of red LED
    int blueVal = 255 - blueIntens / 4.0118;  // Scale to 255 max, taking maximum from Analog in as 1023
    int redVal = 255 - redIntens / 4.0118;

    handleBlinkMode(blueVal, redVal);
    handleContinuousMode(blueVal, redVal);
    handleBlackMode();
    handleButtonPress(blueVal);

    delay(10);  // Delay to allow for other tasks
}

// Handle blink mode with red and blue pulses
void handleBlinkMode(int blueVal, int redVal) {
    if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == HIGH) {
        FastLED.setBrightness(blueVal);
        FastLED.setBrightness(redVal);
        evenColor = CRGB::Red;
        oddColor = CRGB::Blue;
        fillLEDMatrix(evenColor, oddColor);
        digitalWrite(SIGNAL_PIN, HIGH);  // Indicate blue pulse
        delay(PULSE_TIME);
        evenColor = CRGB::Red;
        oddColor = CRGB::Black;
        fillLEDMatrix(evenColor, oddColor);
        digitalWrite(SIGNAL_PIN, LOW);
        delay(BETWEEN_TIME);  // Delay between pulses
    }
}

// Handle continuous mode with red or blue LEDs
void handleContinuousMode(int blueVal, int redVal) {
    if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == LOW) {
        if (digitalRead(BLUE_RED) == HIGH) {
            evenColor = CRGB::Red;
            oddColor = CRGB::Black;
            FastLED.setBrightness(redVal);
        } else {
            evenColor = CRGB::Black;
            oddColor = CRGB::Blue;
            FastLED.setBrightness(blueVal);
        }
        fillLEDMatrix(evenColor, oddColor);
    }
}

// Handle continuous black mode with blue pulse on button press
void handleBlackMode() {
    if (digitalRead(BLINK_PIN) == LOW && digitalRead(LED_BLACK) == HIGH) {
        evenColor = CRGB::Black;
        oddColor = CRGB::Black;
        fillLEDMatrix(evenColor, oddColor);
    }
}

// Handle button press for blue pulse
void handleButtonPress(int blueVal) {
    if (buttonPressed) {
        FastLED.setBrightness(blueVal);
        if (digitalRead(BLINK_PIN) == LOW && digitalRead(LED_BLACK) == HIGH) {
            evenColor = CRGB::Black;
            oddColor = CRGB::Blue;
        } else if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == LOW) {
            evenColor = CRGB::Red;
            oddColor = CRGB::Blue;
        }
        fillLEDMatrix(evenColor, oddColor);
        digitalWrite(SIGNAL_PIN, HIGH);  // Indicate blue pulse
        delay(PULSE_TIME);
        digitalWrite(SIGNAL_PIN, LOW);   // Indicate pulse ended
        if (digitalRead(BLINK_PIN) == LOW && digitalRead(LED_BLACK) == HIGH) {
            evenColor = CRGB::Black;
            oddColor = CRGB::Black;
        } else if (digitalRead(BLINK_PIN) == HIGH && digitalRead(LED_BLACK) == LOW) {
            evenColor = CRGB::Red;
            oddColor = CRGB::Black;
        }
        fillLEDMatrix(evenColor, oddColor);
        buttonPressed = false;
    }
}

// Fill the LED matrix with desired colors
void fillLEDMatrix(CRGB color1, CRGB color2) {
    for (int i = 0; i < NUM_LEDS; i += 3) {
        leds[i] = color1;
        if (i + 1 < NUM_LEDS) {
            leds[i + 1] = color2; leds[i+2] = color2;
        }
    }
    FastLED.show();
}

// Interrupt Service Routine (ISR) for button press
void buttonISR() {
    buttonPressed = true;
}