#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "esp_sleep.h"

// OLED CONFIG
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LORA PINS
#define SS      5
#define RST     4
#define DIO0    15

// GPS
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// BUTTON
#define BUTTON_PIN 27

// BATTERY
#define BATT_PIN 34

// SLEEP SETTINGS
#define uS_TO_S_FACTOR 1000000ULL
#define SLEEP_TIME 10

// RTC VARIABLES
RTC_DATA_ATTR int seq = 0;

// GLOBAL VARIABLES
String gpsStatus = "NO_FIX";
String timeStr = "NO_TIME";

float latitude = 0.0;
float longitude = 0.0;

float battVoltage = 0.0;
int battPercent = 0;

bool displayRequired = false;


// FUNCTION DECLARATIONS
void showOLEDPages();
void readBattery();
void getGPSData();
void transmitData();
void enterDeepSleep();

// SETUP
void setup() {

  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); //Button 

  //OLED
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  }

  display.clearDisplay();
  display.display();

  //  Wake Reason 
  esp_sleep_wakeup_cause_t wakeReason;
  wakeReason = esp_sleep_get_wakeup_cause();

  /*
      Wakeup Causes:
      2 = External GPIO (button)
      3 = Timer
  */

  if (wakeReason == ESP_SLEEP_WAKEUP_EXT0) {
    displayRequired = true;
    Serial.println("Wakeup by BUTTON");
  }
  else if (wakeReason == ESP_SLEEP_WAKEUP_TIMER) {
    displayRequired = false;
    Serial.println("Wakeup by TIMER");
  }
  else {
    // First boot
    displayRequired = true;
    Serial.println("Normal Startup");
  }

  //GPS
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  //LoRa 
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("LoRa Ready");

  
  // MAIN OPERATION CYCLE
  
  seq++;
  getGPSData();
  readBattery();
  transmitData();

  // OLED DISPLAY IF BUTTON WAKE
  if (displayRequired) {
    showOLEDPages();
  }

  
  // GO TO SLEEP
  enterDeepSleep();
}



// LOOP
void loop() {
}


// GPS FUNCTION
void getGPSData() {

  unsigned long start = millis();

  while (millis() - start < 15000) {

    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }

    if (gps.location.isValid()) {

      latitude = gps.location.lat();
      longitude = gps.location.lng();

      gpsStatus = "FIX";

      if (gps.time.isValid()) {

        int hourUTC = gps.time.hour();
        int minuteUTC = gps.time.minute();
        int secondUTC = gps.time.second();

        // Convert UTC to IST (+5:30)
        int hourIST = hourUTC + 5;
        int minuteIST = minuteUTC + 30;

        // Handle minute overflow
        if (minuteIST >= 60) {
        minuteIST -= 60;
        hourIST += 1;
        }

        // Handle hour overflow
        if (hourIST >= 24) {
        hourIST -= 24;
        }

       char buffer[10];
       sprintf(buffer, "%02d:%02d:%02d",
         hourIST,
         minuteIST,
         secondUTC);
       timeStr = String(buffer); 
      }

      return;
    }
  }

  // If timeout occurs

  gpsStatus = "NO_FIX";
  timeStr = "NO_TIME";
}


// BATTERY FUNCTION
void readBattery() {

  int raw = analogRead(BATT_PIN);

  float adcVoltage = (raw / 4095.0) * 3.3;

  battVoltage = adcVoltage * 3.0;

  // Simple 2-cell Li-ion battery percentage estimation
  // 6.6 V = 0% ; 8.4 V = 100%
  battPercent = map(battVoltage * 100,
                    660,
                    840,
                    0,
                    100);

  battPercent = constrain(battPercent, 0, 100);
}


// TRANSMIT FUNCTION
void transmitData() {

  String data =
    String(seq) + "," +
    String(latitude, 6) + "," +
    String(longitude, 6) + "," +
    gpsStatus + "," +
    timeStr + "," +
    String(battPercent);

  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  Serial.println("=================================");
  Serial.println("TRANSMITTED DATA");
  Serial.println(data);
  Serial.println("=================================");
}


// OLED DISPLAY FUNCTION
void showOLEDPages() {

  // ---------- Page 1 ----------

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.println("SEQ:");

  display.setCursor(0, 30);
  display.println(seq);

  display.display();

  delay(3000);

  // ---------- Page 2 ----------

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("GPS STATUS:");

  display.setTextSize(2);

  display.setCursor(0, 30);
  display.println(gpsStatus);

  display.display();

  delay(3000);

  // ---------- Page 3 ----------

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("LATITUDE:");

  display.setCursor(0, 25);
  display.println(latitude, 6);

  display.display();

  delay(3000);

  // ---------- Page 4 ----------

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("LONGITUDE:");

  display.setCursor(0, 25);
  display.println(longitude, 6);

  display.display();

  delay(3000);

  // ---------- Page 5 ----------

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("TIME:");

  display.setTextSize(2);

  display.setCursor(0, 25);
  display.println(timeStr);

  display.display();

  delay(3000);

  // ---------- Page 6 ----------

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("BATTERY:");

  display.setTextSize(2);

  display.setCursor(0, 25);
  display.print(battPercent);
  display.println("%");

  display.display();

  delay(3000);

  // OLED OFF

  display.ssd1306_command(SSD1306_DISPLAYOFF);
}


// DEEP SLEEP FUNCTION
void enterDeepSleep() {

  Serial.println("Entering Deep Sleep...");

  // OLED OFF before sleep
  display.ssd1306_command(SSD1306_DISPLAYOFF);

  // Wakeup by timer
  esp_sleep_enable_timer_wakeup(SLEEP_TIME * uS_TO_S_FACTOR);

  // Wakeup by button (GPIO27)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);

  delay(100);

  esp_deep_sleep_start();
}
