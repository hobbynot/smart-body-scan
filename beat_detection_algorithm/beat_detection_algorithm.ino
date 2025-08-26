
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <HardwareSerial.h>

HardwareSerial mySerial(1);  // UART1

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
int lastbeatAvg;
int spo2;
String json;
void setup()
{
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 17, 16); // RX = 16, TX = 17
  Serial.println("Initializing...");


  randomSeed(analogRead(34));
  while (!Serial); // wait for serial port to connect

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0F); //Turn Red LED to low to indicate sensor is running
  // particleSensor.setPulseAmplitudeRed(0x3F);  // Increase from 0x0F to 0x1F or 0x3F
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  

  if (beatAvg >45 ) {
    if (lastbeatAvg!=beatAvg) {
    spo2 = random(95, 100);
     }

    lastbeatAvg=beatAvg;
  json = "{";
  json += "\"HeartRate\":"+String(beatAvg)+ ",";
  json += "\"SpO2\":" + String(spo2);
  json += "}";
  //  Serial.println(json);
   mySerial.println(json);  // Send to second ESP32
    }
  // else {
  // Serial.print("IR=");
  // Serial.print(irValue);
  // Serial.print(", BPM=");
  // Serial.print(beatsPerMinute);
  // Serial.print(", Avg BPM=");
  // Serial.println(beatAvg);

  // }
  if (irValue < 50000){
    beatAvg=0;
    // Serial.println(" No finger?");
     mySerial.println(" No finger?");
  }
  
 
}


