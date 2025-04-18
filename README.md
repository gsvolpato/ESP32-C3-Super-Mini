# ESP32-C3 BLE Serial

This project implements a BLE UART service on the ESP32-C3 Super Mini board. It creates a wireless serial connection over Bluetooth Low Energy, allowing bidirectional communication between the ESP32-C3 and a BLE client (like a smartphone).

## Features

- Implements standard BLE UART service
- LED status indication (blinking when disconnected, solid when connected)
- Sends "hello" message every second when connected
- Prints received messages to Serial monitor
- Maximum power output for better range

## Hardware Requirements

- ESP32-C3 Super Mini board
- USB cable for programming and serial monitor

## Software Requirements

- PlatformIO
- Arduino framework for ESP32
- NimBLE-Arduino library

## Setup

1. Clone this repository
2. Open in PlatformIO
3. Build and upload to your ESP32-C3 board
4. Use any BLE UART client app to connect (search for "ESP32-C3-BLE-Serial" device)

## Usage

1. The device will advertise as "ESP32-C3-BLE-Serial"
2. Connect using any BLE UART client
3. The onboard LED will indicate connection status
4. The device will send "hello" every second
5. Any messages sent to the device will be printed to the Serial monitor

## License

MIT License 