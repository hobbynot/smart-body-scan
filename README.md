# Smart Body Scan

ESP32-based project for measuring height, weight, and heart detection.  
Two ESP32 boards:
- **E1 (Main Board):** Calculates height & weight, communicates with PC software.
- **E2 (Heart Detection Board):** Runs heart detection algorithm, sends data to E1.

## Features
- Real-time heart detection
- Serial communication with custom software
- Modular ESP32 setup

## Hardware
- 2 × ESP32
- Sensors for height & weight
- Heart detection sensor

## Usage
1. Upload `E1` code to main ESP32.
2. Upload `E2` code to heart detection ESP32.
3. Connect E2 TX → E1 RX.
4. Run PC software via USB on E1.

---
Made with ❤️ by Sanjiv
