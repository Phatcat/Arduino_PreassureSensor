#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

float peakValue = 0.0;
float previousValue = 0;
unsigned int zeroValue = 0;
bool switchReset = false;
bool switchCalib = true;
bool doUpdateDisplay = true;

void setup()
{
  lcd.begin(16, 2);
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  Serial.begin(9600);
}

void loop()
{
  if(digitalRead(3) == HIGH)
  {
    peakValue = 0;
    switchReset = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Resetting...");
    doUpdateDisplay = true;
  }
  
  if(digitalRead(2) == HIGH)
  {
    zeroValue = analogRead(A0);
    switchCalib = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
    doUpdateDisplay = true;
  }

  do {;} while(digitalRead(2) == HIGH || digitalRead(3) == HIGH);

  unsigned int currentValue = analogRead(A0);

  if(switchCalib)
  {
    zeroValue = currentValue;
    switchCalib = false;
  }

  if(switchReset)
  {
    peakValue = 0;
    switchReset = false;
  }

  // Get delta
  int deltaBits = int(currentValue) - int(zeroValue);

  // Get delta in voltage
  float deltaVoltage = (float(deltaBits) / 1024.0) * 5.0;

  // Calculate voltage delta into kPa
  float kpaValue = ((deltaVoltage / 5.0) - 0.04) / 0.018;

  if(kpaValue > peakValue)
  {
    peakValue = kpaValue;
    doUpdateDisplay = true;
  }

  if(kpaValue > (previousValue + 0.1) || kpaValue < (previousValue - 0.1))
    doUpdateDisplay = true;

  // display what is being read on A0 right now
  Serial.println(currentValue);
  Serial.print(",");
  // display the calculated value in kPa
  Serial.println(kpaValue);

  if(doUpdateDisplay)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Peak: ");
    lcd.print(peakValue, 3);
    lcd.print(" kPa");
    lcd.setCursor(0, 1);
    lcd.print("Current: ");
    lcd.print(kpaValue, 4);
    doUpdateDisplay = false;
  }

  previousValue = kpaValue;
}
