#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------------------- Pin Definitions --------------------
const byte TRIG_PIN = 9;
const byte ECHO_PIN = 10;
const byte PIR_PIN  = 7;

const int MAX_DISTANCE = 300;      // cm
const int TOTAL_READINGS = 5;

const unsigned long PIR_CONFIRM_TIME = 200;     // ms
const unsigned long DISPLAY_TIME = 2000;        // ms
const unsigned long PIR_LOW_TIMEOUT = 10000;    // ms

// -------------------- State Machine --------------------
enum SystemState
{
  STANDBY,
  MEASURE,
  DISPLAY_RESULT,
  WAIT_PIR_LOW
};

SystemState currentState = STANDBY;

// -------------------- Variables --------------------
bool distanceValid = false;
float finalDistance = 0;

bool outOfRange = false;

// ----------------- Function Prototypes ---------------
bool confirmPIR();

bool getDistance(float &distance);

float getFilteredDistance();

void displayHumanDetected(float distance);

void displayOutOfRange();

void waitForPIRLow();

void clearOLED();

// --------------- SETUP -------------------
void setup()
{
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(PIR_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  Wire.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.display();

  clearOLED();

  Serial.begin(9600);
}

// ----------------- LOOP ---------------------
void loop()
{
  switch (currentState)
  {

    // ----------- STANDBY --------------------
    case STANDBY:
      clearOLED();
      if (confirmPIR())
      {
        currentState = MEASURE;
      }
      break;

    // -------- MEASUREMENT ---------------------
    case MEASURE:

  for (int i = 0; i < 5; i++)
  {
    float d;

    if (getDistance(d))
    {
      Serial.print("Reading ");
      Serial.print(i + 1);
      Serial.print(" = ");
      Serial.println(d);
    }
    else
    {
      Serial.print("Reading ");
      Serial.print(i + 1);
      Serial.println(" = INVALID");
    }

    delay(100);
  }

  finalDistance = getFilteredDistance();

  Serial.print("Final Distance = ");
  Serial.println(finalDistance);

  if (finalDistance < 0)
  {
    distanceValid = false;
  }
  else
  {
    distanceValid = true;
  }

  currentState = DISPLAY_RESULT;
  break;
    // --------------- DISPLAY --------------------
    case DISPLAY_RESULT:

      if (distanceValid)
      {
        displayHumanDetected(finalDistance);
      }
      else
      {
        displayOutOfRange();
      }

      currentState = WAIT_PIR_LOW;

      break;

    // -------------- WAIT UNTIL PIR RETURNS LOW -----------------
    case WAIT_PIR_LOW:

      waitForPIRLow();

      currentState = STANDBY;

      break;
  }
}

// ---------- PIR CONFIRMATION --------------------------
bool confirmPIR()
{
  if (digitalRead(PIR_PIN) == LOW)
    return false;

  unsigned long startTime = millis();

  while (millis() - startTime < PIR_CONFIRM_TIME)
  {
    if (digitalRead(PIR_PIN) == LOW)
      return false;
  }

  return true;
}

bool getDistance(float &distance)
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Timeout corresponds to ~300 cm
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0)
  {
    return false;
  }

  distance = duration/ 58.2;
  Serial.print("Duration = ");
  Serial.print(duration);
  Serial.print(" us   Distance = ");
  Serial.println(distance);

  if (distance <= 0 || distance > MAX_DISTANCE)
  {
    return false;
  }

  return true;
}

float getFilteredDistance()
{
  float readings[TOTAL_READINGS];
  int validCount = 0;

  // Take EXACTLY 5 measurements
  for (int i = 0; i < TOTAL_READINGS; i++)
  {
    float d;

    if (getDistance(d))
    {
      readings[validCount] = d;
      validCount++;
    }

    delay(60);
  }

  // Less than 3 valid readings = Out of Range
  if (validCount < 3)
  {
    return -1;
  }

  // If all 5 readings are valid, use trimmed mean
  if (validCount == 5)
  {
    // Bubble Sort
    for (int i = 0; i < validCount - 1; i++)
    {
      for (int j = 0; j < validCount - i - 1; j++)
      {
        if (readings[j] > readings[j + 1])
        {
          float temp = readings[j];
          readings[j] = readings[j + 1];
          readings[j + 1] = temp;
        }
      }
    }

    return (readings[1] + readings[2] + readings[3]) / 3.0;
  }

  // Average valid readings (3 or 4 valid)
  float sum = 0;

  for (int i = 0; i < validCount; i++)
  {
    sum += readings[i];
  }

  return sum / validCount;
}

// DISPLAY
// HUMAN DETECTED
void displayHumanDetected(float distance)
{
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 8);
  display.println("Human");

  display.setCursor(0, 32);
  display.println("Detected");

  display.display();

  delay(DISPLAY_TIME);

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(0, 8);
  display.println("Distance");

  display.setTextSize(2);

  display.setCursor(25, 36);
  display.print(distance, 1);
  display.print(" cm");

  display.display();

  delay(DISPLAY_TIME);

  clearOLED();
}

// DISPLAY
// OUT OF RANGE
void displayOutOfRange()
{
  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(0, 10);
  display.println("Motion");

  display.setCursor(0, 34);
  display.println("Detected");

  display.display();

  delay(DISPLAY_TIME);

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(0, 20);
  display.println("Out of");

  display.setCursor(15, 45);
  display.println("Range");

  display.display();

  delay(DISPLAY_TIME);

  clearOLED();
}

// WAIT UNTIL PIR RETURNS LOW
// Timeout = 10 Seconds
void waitForPIRLow()
{
  unsigned long start = millis();

  while (digitalRead(PIR_PIN) == HIGH)
  {
    if (millis() - start >= PIR_LOW_TIMEOUT)
    {
      break;
    }

    delay(20);
  }

  clearOLED();
}

// OLED OFF
void clearOLED()
{
  display.clearDisplay();
  display.display();
}
