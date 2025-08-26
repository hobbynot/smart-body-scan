# ESP32 Height & Weight Monitor with Heart Detection

## 📌 Overview
This project uses two ESP32 boards:
- **E1 (Main Board):** Handles weight (HX711) and height (Ultrasonic sensor) + forwards data to PC/web.
- **E2 (Heart Board):** Runs heart detection algorithm and streams results via UART to E1.

The system provides live readings via:
- Serial (USB to PC software)
- Web server (ESP32 AP)

## ⚡ Features
- Weight measurement with HX711
- Height measurement with HC-SR04
- Continuous heart data from E2
- Web dashboard for calibration & live view
- Data streaming in JSON

## 🛠️ Hardware
- ESP32 x 2
- HX711 + Load Cell
- Ultrasonic sensor (HC-SR04)
- Power via USB

## 🚀 Usage
1. Flash `E1_Main/E1_Main.ino` on ESP32 main board.
2. Flash `E2_Heart/E2_Heart.ino` on ESP32 heart board.
3. Connect E2 TX → E1 RX.
4. Open serial monitor or connect to Wi-Fi AP `ESP_Height_Weight_2`.

## 📷 Images
(Add wiring diagrams/screenshots here)

## 📜 License
MIT License
