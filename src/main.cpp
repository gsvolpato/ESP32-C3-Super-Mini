#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

// UUIDs for BLE UART service (standard UART service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Pin definition
#define LED_PIN 0  // GPIO 0 for the onboard LED

// Global variables
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;  // Interval to send "hello" (1 second)
unsigned long ledMillis = 0;
const long ledBlinkInterval = 250;  // Blink every 250ms
bool ledState = false;

// Device name - this is what shows up in the BLE scanner
const char* deviceName = "ESP32-C3-BLE-Serial";

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        deviceConnected = true;
        Serial.println("Client connected");
        // Turn LED on solid when connected
        digitalWrite(LED_PIN, HIGH);
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Client disconnected");
    }
};

class RxCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            Serial.print("Received: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.print(rxValue[i]);
            }
            Serial.println();
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("BLE UART Service starting...");
    
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize NimBLE
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Set maximum power

    // Create the BLE Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create the BLE Service
    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic for TX (ESP32 -> Phone)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );

    // Create a BLE Characteristic for RX (Phone -> ESP32)
    NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
    );
    pRxCharacteristic->setCallbacks(new RxCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // helps with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();

    Serial.println("BLE UART Service started. Waiting for connections...");
}

void loop() {
    unsigned long currentMillis = millis();

    // Blink LED until connected
    if (!deviceConnected) {
        if (currentMillis - ledMillis >= ledBlinkInterval) {
            ledMillis = currentMillis;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
    }

    // If device is connected, send "hello" every second
    if (deviceConnected && (currentMillis - previousMillis >= interval)) {
        previousMillis = currentMillis;
        
        // Send "hello" with line ending to make each message appear on a new line
        std::string helloMsg = "hello\n";
        pTxCharacteristic->setValue(helloMsg);
        pTxCharacteristic->notify();
        
        Serial.println("Sent: hello");
    }

    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the Bluetooth stack time to get ready
        pServer->startAdvertising(); // Restart advertising
        Serial.println("Started advertising again");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        // Connected
        oldDeviceConnected = deviceConnected;
    }
} 