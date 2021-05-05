#include <U8g2lib.h>
#include <SPI.h>

// Define the number of samples to average out
#define AVERAGE 32

// Define the number of pixels on the screen to use for the graph (x-axis)
#define PIXELCOUNT 118

// Set and use the correct mode for the display (ST7920, SPI)
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);

// Create array for graphical display of sensor value in a graph
unsigned int graphArray[PIXELCOUNT] = {0};

unsigned int currentValue = 0;
unsigned int i = 1;
float peakKpa = 0.0;
float zeroVoltage = 0.0;
bool switchReset = false;
bool switchCalib = true;

void setup()
{
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  Serial.begin(9600); 
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setColorIndex(1);
}

void loop()
{
  u8g2.firstPage();
  do { draw(); calculate(); } while( u8g2.nextPage() );
}

void draw()
{
  if(digitalRead(3) == LOW)
    switchReset = true;
  
  if(digitalRead(2) == LOW)
    switchCalib = true;

  do {;} while(digitalRead(2) == LOW || digitalRead(3) == LOW);

  // Draw frames
  u8g2.drawLine(9, 0, 9, 55);
  u8g2.drawLine(0, 55, 127, 55);
      
  // Fill in reference points
  u8g2.setFont(u8g2_font_blipfest_07_tr);
      
  u8g2.drawPixel(8, 2);
  u8g2.drawStr(0, 5, "50");
      
  u8g2.drawPixel(8, 12);
  u8g2.drawStr(0, 15, "40");
      
  u8g2.drawPixel(8, 22);
  u8g2.drawStr(0, 25, "30");
      
  u8g2.drawPixel(8, 32);
  u8g2.drawStr(0, 35, "20");
      
  u8g2.drawPixel(8, 42);
  u8g2.drawStr(1, 45, "10");
      
  u8g2.drawPixel(8, 52);
  u8g2.drawStr(4, 54, "0");
      
  // Set a font fitting to the bottom frame
  u8g2.setFont(u8g2_font_courB08_tr);
  // Fill in text
  u8g2.drawStr(1, 64, "Peak kPa: ");
  char peak[10];
  dtostrf(peakKpa, 0, 8, peak);
  u8g2.drawStr(58, 64, peak);

  // Draw the graph from the array with a 0-point at pixel y = 52 and an absolute y-point of 10 (next to the border line)
  for(int j = 0; j < PIXELCOUNT; j++)
    u8g2.drawPixel(10 + j, 52 - graphArray[j]);
}

void calculate()
{
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

      for(int j = 0; j < PIXELCOUNT; j++)
        graphArray[j] = 0;

      switchCalib = false;
    }
  
    if(switchReset)
    {
      peakKpa = 0;

      for(int j = 0; j < PIXELCOUNT; j++)
        graphArray[j] = 0;

      switchReset = false;
    }
  
    // Get current reading in voltage
    float currentVoltage = (float(currentValue) / 1024.0) * 5.0;
  
    // Calculate voltage into kPa
    float currentKpa = (((currentVoltage / 5.0) - 0.04) / 0.018) - (((zeroVoltage / 5.0) - 0.04) / 0.018);
  
    if(currentKpa > peakKpa)
      peakKpa = currentKpa;

    // display what is being read on A0 right now - used for debugging
    // Serial.println(currentValue);
    // Serial.print(",");

    // display the calculated value in kPa
    // Serial.println(currentKpa);

    for(int j = 0; j < (PIXELCOUNT - 1); j++)
      graphArray[j] = graphArray[j + 1];

    graphArray[PIXELCOUNT - 1] = int(currentKpa);

    currentValue = 0;
    i = 1;
  }
  else
    i++;
}
