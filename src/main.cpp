#include <Arduino.h>

const int LED_PIN = 8;
const int BUTTON_PIN = 9;
bool ledState = false;
bool lastButtonState = HIGH;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  if (currentButtonState != lastButtonState) {
    if (currentButtonState == HIGH) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    } 
    lastButtonState = currentButtonState;
  }
  
  delay(10);
} 