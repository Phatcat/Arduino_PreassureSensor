#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// Define the number of samples to average out
#define AVERAGE 128

unsigned int currentValue = 0;
unsigned int i = 1;
float peakKpa = 0.0;
float zeroVoltage = 0.0;
float previousKpa = 0.0;
bool switchReset = false;
bool switchCalib = true;
bool doUpdateDisplay = true;

void setup()
{
  lcd.begin(16, 2);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  Serial.begin(9600);
}

void loop()
{
  if(digitalRead(3) == LOW)
  {
    switchReset = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Resetting...");
    doUpdateDisplay = true;
  }
  
  if(digitalRead(2) == LOW)
  {
    switchCalib = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
    doUpdateDisplay = true;
  }

  do {;} while(digitalRead(2) == LOW || digitalRead(3) == LOW);

  // Get current reading on A0
  currentValue += analogRead(A0);

  if(i >= AVERAGE)
  {
    currentValue /= AVERAGE;

    if(switchCalib)
    {
      unsigned int zeroValue = currentValue;
      // Get zero value in voltage
      zeroVoltage = (float(zeroValue) / 1024.0) * 5.0;
      switchCalib = false;
    }
  
    if(switchReset)
    {
      peakKpa = 0;
      switchReset = false;
    }
  
    // Get current reading in voltage
    float currentVoltage = (float(currentValue) / 1024.0) * 5.0;
  
    // Calculate voltage into kPa
    float currentKpa = (((currentVoltage / 5.0) - 0.04) / 0.018) - (((zeroVoltage / 5.0) - 0.04) / 0.018);
  
    if(currentKpa > peakKpa)
    {
      peakKpa = currentKpa;
      doUpdateDisplay = true;
    }
  
    if(currentKpa != previousKpa)
    {
      previousKpa = currentKpa;
      doUpdateDisplay = true;
    }

    // display what is being read on A0 right now - used for debugging
    // Serial.println(currentValue);
    // Serial.print(",");

    // display the calculated value in kPa
    Serial.println(currentKpa);

    if(doUpdateDisplay)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Peak: ");
      lcd.print(peakKpa, 3);
      lcd.print(" kPa");
      lcd.setCursor(0, 1);
      lcd.print("Current: ");
      lcd.print(currentKpa, 4);
      doUpdateDisplay = false;
    }

    currentValue = 0;
    i = 1;
  }
  else
    i++;
}
