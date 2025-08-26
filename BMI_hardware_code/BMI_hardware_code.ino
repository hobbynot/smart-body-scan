#include <WiFi.h>
#include <WebServer.h>
#include <HX711_ADC.h>
#include <HardwareSerial.h>
#include <Preferences.h>
Preferences preferences;

HardwareSerial mySerial(2);  // UART2
bool usbConnected = false;
// Wi-Fi credentials
const char* ssid = "ESP_Height_Weight_2";
const char* password = "12345678";

// **ESP32 HX711 Pins**
const int HX711_dout = 22;  // DOUT -> ESP32 GPIO4
const int HX711_sck = 21;   // SCK  -> ESP32 GPIO5

// HC-SR04 Pins
#define TRIG_PIN 16
#define ECHO_PIN 17

// Calibration
float calibrationFactor = -22690.65;
bool tareCompleted = false;
float fixedHigh = 185.0;
float offset = 0.0;

// Objects
HX711_ADC LoadCell(HX711_dout, HX711_sck);
WebServer server(80);


struct SaveResult {
  bool success;
  int savedValue;
  String message;
};

// This function checks and saves/updates the number


// -------------------- Sensor Functions ----------------------

float measureHeight() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = (duration * 0.0343) / 2;
  return distance;
}

float getAverageHeight() {
  float sum = 0;
  const int samples = 10;
  for (int i = 0; i < samples; i++) {
    sum += measureHeight();
    delay(30);
  }
  return sum / samples;
}

// -------------------- Web Handlers ----------------------

void handleRoot() {
  String html = R"rawliteral(
   <!DOCTYPE html>
  <html>
  <head>
  <title>ESP32 Height & Weight Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root {
      --primary: #2196F3;
      --success: #4CAF50;
      --dark: #333;
      --light: #f5f5f5;
      --shadow: 0 1px 3px rgba(0,0,0,0.12);
    }
    
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 16px;
      background-color: var(--light);
      color: var(--dark);
      max-width: 600px;
      margin: 0 auto;
    }
    
    .header {
      text-align: center;
      margin-bottom: 24px;
      padding-bottom: 12px;
      border-bottom: 1px solid #ddd;
    }
    
    .card {
      background: white;
      border-radius: 8px;
      padding: 16px;
      margin-bottom: 16px;
      box-shadow: var(--shadow);
    }
    
    .reading {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 8px;
    }
    
    .reading-label {
      font-size: 1.1em;
      font-weight: bold;
    }
    
    .reading-value {
      font-size: 1.8em;
      font-weight: bold;
      color: var(--primary);
    }
    
    .reading-unit {
      font-size: 0.9em;
      margin-left: 4px;
      color: #777;
    }
    
    .controls {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    
    .input-group {
      display: flex;
      gap: 8px;
    }
    
    input {
      flex: 1;
      padding: 10px;
      border: 1px solid #ddd;
      border-radius: 4px;
      font-size: 0.9em;
    }
    
    input:focus {
      outline: none;
      border-color: var(--primary);
    }
    
    button {
      padding: 10px;
      background-color: var(--primary);
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-weight: bold;
      transition: background-color 0.2s;
    }
    
    button:hover {
      background-color: #1976D2;
    }
    
    .btn-full {
      width: 100%;
    }
    
    .section-title {
      font-size: 1em;
      color: #777;
      margin-top: 4px;
      margin-bottom: 8px;
    }
    
    .status {
      text-align: center;
      padding: 4px;
      font-size: 0.8em;
      color: #777;
      height: 18px;
    }
  </style>
    </head>
      <body>
        <div class="header">
          <h2>Height & Weight Monitor</h2>
        </div>
  
        <div class="card">
          <div class="reading">
            <span class="reading-label">Height</span>
              <div>
                <span class="reading-value" id="height">--</span>
                <span class="reading-unit">cm</span>
              </div>
          </div>
          <div class="reading">
            <span class="reading-label">Weight</span>
              <div>
                <span class="reading-value" id="weight">--</span>
                <span class="reading-unit">kg</span>
              </div>
          </div>
        </div>
  
        <div class="card">
          <div class="section-title">Weight Calibration</div>
            <div class="controls">
              <button class="btn-full" id="tareBtn">Tare Scale</button>
              <div class="input-group">
                <input id="mass" type="number" placeholder="Known Mass (kg)">
                <button id="calibrateBtn">Calibrate</button>
              </div>
            </div>
          </div>
        </div>
  
        <div class="card">
          <div class="section-title">Height Configuration</div>
            <div class="controls">
                <button class="btn-full" id="calibHeightBtn">Calibrate Height Sensor (No Human)</button>
                <div class="input-group">
                  <input id="offset" type="number" placeholder="Height Offset (cm)">
                  <button id="offsetBtn">Set Offset</button>
                </div>
                <div class="input-group">
                  <input id="fixedHeight" type="number" placeholder="Fixed Height (cm)">
                  <button id="fixHeightBtn">Save Height</button>
                </div>
            </div>
          </div>
        </div>
  
        <div class="status" id="status"></div>

  <script>
    // Elements
    const heightEl = document.getElementById('height');
    const weightEl = document.getElementById('weight');
    const statusEl = document.getElementById('status');
    
    // Buttons
    document.getElementById('tareBtn').addEventListener('click', () => sendCommand('/tare'));
    document.getElementById('calibrateBtn').addEventListener('click', () => {
      const mass = document.getElementById('mass').value;
      if (!mass) {
        showStatus('Please enter a known mass');
        return;
      }
      sendCommand('/calibrate?mass=' + mass);
    });
    
    document.getElementById('calibHeightBtn').addEventListener('click', () => sendCommand('/calibrateHeight'));
    document.getElementById('offsetBtn').addEventListener('click', () => {
      const offset = document.getElementById('offset').value;
      if (!offset) {
        showStatus('Please enter an offset value');
        return;
      }
      sendCommand('/offset?value=' + offset);
    });
    
    document.getElementById('fixHeightBtn').addEventListener('click', () => {
      const height = document.getElementById('fixedHeight').value;
      if (!height) {
        showStatus('Please enter a height value');
        return;
      }
      sendCommand('/handleMemorySave?value=' + height);
    });
    
    // Functions
    function sendCommand(path) {
      fetch(path)
        .then(response => response.text())
        .then(message => {
          showStatus(message);
        })
        .catch(error => {
          showStatus('Error: ' + error.message);
        });
    }
    
    function showStatus(message) {
      statusEl.textContent = message;
      setTimeout(() => {
        statusEl.textContent = '';
      }, 3000);
    }
    
    function updateReadings() {
      fetch('/data')
        .then(response => {
          if (!response.ok) throw new Error('Network response error');
          return response.json();
        })
        .then(data => {
          heightEl.textContent = data.humanHeight.toFixed(1);
          weightEl.textContent = data.weight.toFixed(2);
        })
        .catch(error => {
          console.error('Error fetching data:', error);
          // Don't stop trying to update if there's an error
        });
    }
    
          // Initial update and set interval
          updateReadings();
        const updateInterval = setInterval(updateReadings, 1000); // Changed to 1 second to reduce resource usage
      </script>
    </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleTare() {
  LoadCell.tareNoDelay();
  tareCompleted = false;
  server.send(200, "text/plain", "Taring...");
}

void handleCalibrateWeight() {
  if (server.hasArg("mass")) {
    float knownMass = server.arg("mass").toFloat();
    if (knownMass > 0) {
      LoadCell.refreshDataSet();
      calibrationFactor = LoadCell.getNewCalibration(knownMass);
      LoadCell.setCalFactor(calibrationFactor);
      server.send(200, "text/plain", "Calibration Factor: " + String(calibrationFactor));
    } else {
      server.send(400, "text/plain", "Invalid Mass");
    }
  } else {
    server.send(400, "text/plain", "Mass not provided");
  }
}

void handleCalibrateHeight() {
  fixedHigh = getAverageHeight();
  server.send(200, "text/plain", "Height Calibrated to: " + String(fixedHigh, 1) + " cm");
}

// struct SaveResult {
//   bool success;
//   int savedValue;
//   String message;
// };

// This function checks and saves/updates the number
SaveResult saveOrUpdateFixedHeight(int number) {
  SaveResult result;

  if (!preferences.begin("myData", false)) {
    result.success = false;
    result.savedValue = -1;
    result.message = "Failed to open preferences namespace.";
    return result;
  }

  int currentValue = preferences.getInt("fixed_height", -1);

  if (currentValue == -1) {
    preferences.putInt("fixed_height", number);
    result.message = "Key not found. New value saved.";
  } else {
    preferences.putInt("fixed_height", number);
    result.message = "Key found. Value overwritten.";
  }

  int confirmValue = preferences.getInt("fixed_height", -1);
  preferences.end();  // Always close preferences

  if (confirmValue == number) {
    result.success = true;
    result.savedValue = confirmValue;
  } else {
    result.success = false;
    result.savedValue = confirmValue;
    result.message = "Value not saved properly or mismatch.";
  }

  return result;
}

void handleMemorySave() {
  if (server.hasArg("value")) {
    int inputValue = server.arg("value").toInt();

    SaveResult result = saveOrUpdateFixedHeight(inputValue);

    if (result.success) {
      fixedHigh = result.savedValue;
      server.send(200, "text/plain",
                  "Saved successfully. Value: " + String(result.savedValue));
    } else {
      server.send(500, "text/plain",
                  "Failed to save. Reason: " + result.message);
    }
  } else {
    server.send(400, "text/plain", "Missing 'value' parameter");
  }
}

void handleSetOffset() {
  if (server.hasArg("value")) {
    offset = server.arg("value").toFloat();
    server.send(200, "text/plain", "Offset set to " + String(offset, 1) + " cm");
  } else {
    server.send(400, "text/plain", "Missing 'value' parameter");
  }
}

void handleGetData() {
  float humanHeight = fixedHigh - (measureHeight() + offset);
  float weight = LoadCell.update() ? LoadCell.getData() : 0.0;
  weight= weight + 2.5;

  String json = "{";
  json += "\"fixedHigh\":" + String(fixedHigh, 1) + ",";
  json += "\"offset\":" + String(offset, 1) + ",";
  json += "\"humanHeight\":" + String(humanHeight, 1) + ",";
  json += "\"weight\":" + String(weight, 2);
  json += "}";

  // if (weight > 30 && weight < 150) {
    server.send(200, "application/json", json);
  // }
}

void getSerialData() {
  float humanHeight = fixedHigh - (measureHeight() + offset);
  float weight = LoadCell.update() ? LoadCell.getData() : 0.0;
  // Serial.println(weight);
  weight= weight + 2.5;
  String jsonVitals;
  if (mySerial.available()) {
    jsonVitals = mySerial.readStringUntil('\n');
    // Serial.println(jsonVitals);
  }
  String json = "{";
  json += "\"fixedHigh\":" + String(fixedHigh, 1) + ",";
  json += "\"offset\":" + String(offset, 1) + ",";
  json += "\"humanHeight\":" + String(humanHeight, 1) + ",";
  json += "\"weight\":" + String(weight, 2);
  json += "\"jsonVitals\":" + String(jsonVitals);
  json += "}";

  if (weight > 30 && weight < 150 /*&& humanHeight < fixedHigh*/) {
    Serial.println(json);
  }
}

void resetMySerial() {
  mySerial.end();
  delay(50);
  mySerial.begin(115200, SERIAL_8N1, 19, 18);
  while (mySerial.available()) mySerial.read();  // flush garbage
  Serial.println("UART2 reinitialized after reconnect");
}

// -------------------- Setup & Loop ----------------------

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 19, 18);  // RX = 19, TX = 18
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi AP: " + String(ssid));
  Serial.println("IP: " + WiFi.softAPIP().toString());

  // LoadCell
  LoadCell.begin();
  LoadCell.setCalFactor(calibrationFactor);
  LoadCell.start(2000, true);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("HX711 Timeout! Check wiring.");
    while (1)
      ;
  }

  //height calibration
  // Initialize Preferences
  if (preferences.begin("myData", true)) {              // true = read-only
    int data = preferences.getInt("fixed_height", -1);  // Note: Key name must match the one used in saving

    if (data != -1) {
      fixedHigh = data;
      Serial.println("fixedHeight key exists. Value: " + String(fixedHigh));
    } else {
      Serial.println("fixedHeight key does not exist.");
    }

    preferences.end();
  } else {
    Serial.println("Failed to open preferences");
  }

  // Web routes
  server.on("/", handleRoot);
  server.on("/tare", handleTare);
  server.on("/calibrate", handleCalibrateWeight);
  server.on("/calibrateHeight", handleCalibrateHeight);
  server.on("/offset", handleSetOffset);
  server.on("/data", handleGetData);
  server.on("/handleMemorySave", handleMemorySave);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
 
   // Detect PC connection change
  if (Serial && !usbConnected) {
    usbConnected = true;
    resetMySerial();
  } else if (!Serial && usbConnected) {
    usbConnected = false;  // mark as disconnected
  }
  getSerialData();
  server.handleClient();
  // handleGetData();

  //  }


  if (LoadCell.getTareStatus() && !tareCompleted) {
    Serial.println("Tare completed.");
    tareCompleted = true;
  }
  delay(10);
}
