#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NimBLEDevice.h>
#include "image_list.h"  

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_RETURN 9
#define BUTTON_ENTER 10
#define OLED_SDA 6
#define OLED_SCL 7

void setup() {
    pinMode(BUTTON_RETURN, INPUT_PULLUP);
    pinMode(BUTTON_ENTER, INPUT_PULLUP);

    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.display();
    delay(2000);

    display.clearDisplay();
    display.drawBitmap(0, 0, ble_spam3, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.display();

    NimBLEDevice::init("BLE_Spam_Device");
}

void startBLESpam() {
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setScanResponse(true);
    pAdvertising->addData(NimBLEAdvertisementData());
    pAdvertising->start();

    while (true) {
        if (digitalRead(BUTTON_RETURN) == LOW) {
            ESP.restart();  // Restart the board during the attack
        }
        delay(1000);  // Keep advertising
    }
}

void loop() {
    if (digitalRead(BUTTON_ENTER) == LOW) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("Begin BLE spam?");
        display.println("1. Press Enter to start.");
        display.println("2. Press Return to restart.");
        display.display();

        while (true) {
            if (digitalRead(BUTTON_ENTER) == LOW) {
                startBLESpam();
            }
            if (digitalRead(BUTTON_RETURN) == LOW) {
                ESP.restart();  // Restart the board
            }
        }
    }
}
