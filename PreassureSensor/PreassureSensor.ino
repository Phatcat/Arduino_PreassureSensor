#include <U8g2lib.h>

// Define the number of samples to average out
#define AVERAGE 32

// Define the number of pixels on the screen to use for the graph (x-axis)
#define PIXELCOUNT 118

// Set and use the correct mode for the display (ST7920, 128x64, SPI)
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);

// Create array for graphical display of sensor value in graph form and nullify all entries
float graphArray[PIXELCOUNT] = {0.0};

unsigned int zeroPixelY = 52;
int scale = 10;

unsigned int currentValue = 0;
unsigned int i = 1;
float peakKpa = 0.0;
float zeroVoltage = 0.0;

bool firstFill = true;
unsigned int fillCounter = 0;

bool holdGraph = false;

void setup()
{
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  Serial.begin(9600); 
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setColorIndex(1);

  // Initial calibration of sensor input
  drawConfig(true);
}

void loop()
{
  u8g2.firstPage();
  do
  {
    drawBars();

    if(digitalRead(4) == LOW)
    {
      // Hault the program to make sure the state only switches once per button press
      do { ; } while ( digitalRead(4) == LOW );
      // Switch hold state on button release
      holdGraph = !holdGraph;
    }

    if(digitalRead(2) == LOW)
      drawConfig(true);
    else if(digitalRead(3) == LOW)
      drawConfig(false);
    else
    {
      drawGraph();
      if(!holdGraph)
        calculate();
    }
  } while( u8g2.nextPage() );
}

void drawBars()
{
/*********** Draw border frames *************/
  u8g2.drawLine(9, 0, 9, 55);
  u8g2.drawLine(9, 55, 127, 55);
/******* Draw reference pixel points ********/
  u8g2.drawPixel(8, 2);
  u8g2.drawPixel(8, 12);
  u8g2.drawPixel(8, 22);
  u8g2.drawPixel(8, 32);
  u8g2.drawPixel(8, 42);
  u8g2.drawPixel(8, 52);
/********************************************/

  // Set font for graph reference points
  u8g2.setFont(u8g2_font_blipfest_07_tr);

/************ Reference values **************/
  int k = 4;
  if(scale < 5)
    k = 0;
  else if(scale < 10)
    k = 1;
  u8g2.setCursor(k, 5);
  u8g2.print(50 / scale);

  k = 4;
  if(scale < 5)
    k = 0;
  u8g2.setCursor(k, 15);
  u8g2.print(40 / scale);

  k = 4;
  if(scale < 2)
    k = 0;
  else if(scale < 5)
    k = 1;
  u8g2.setCursor(k, 25);
  u8g2.print(30 / scale);

  k = 4;
  if(scale < 2)
    k = 0;
  else if(scale < 5)
    k = 1;
  u8g2.setCursor(k, 35);
  u8g2.print(20 / scale);

  k = 4;
  if(scale < 2)
    k = 1;
  u8g2.setCursor(k, 45);
  u8g2.print(10 / scale);

  u8g2.setCursor(4, 55);
  u8g2.print(0);
/********************************************/

  u8g2.setCursor(0, 64);
  if(holdGraph)
    u8g2.print("HO");
  else
    u8g2.print("GO");

  // Set font fitting to the bottom frame
  u8g2.setFont(u8g2_font_courB08_tr);

  // Fill in text and peak value
  u8g2.setCursor(12, 64);
  u8g2.print("Peak kPa: ");
  u8g2.setCursor(67, 64);
  u8g2.print(peakKpa, 7);
}

void drawGraph()
{
  // Draw the graph from the array with a 0-point at zeroPixelY and an absolute leftmost x-point of 10 (horizontal border line + 1)
  for(int j = 0; j < (firstFill ? fillCounter : PIXELCOUNT); j++)
  {
    // Pixel graph
    // u8g2.drawPixel(10 + j, zeroPixelY - round(graphArray[j]));

    // Bar graph
    // u8g2.drawLine(10 + j, zeroPixelY, 10 + j, zeroPixelY - round(graphArray[j]));

/***** Continuous line graph ****************/
    u8g2.drawPixel(10 + j, zeroPixelY - round(graphArray[j]));
    if(j > 0)
      u8g2.drawLine(9 + j, zeroPixelY - round(graphArray[j - 1]), 10 + j, zeroPixelY - round(graphArray[j]));
/********************************************/
  }
}

void drawConfig(bool calibrating)
{
  // Reset peak
  peakKpa = 0.0;

  // reset scale to 0 - 5 kPa
  scale = 10;

  for(int j = 0; j < PIXELCOUNT; j++)
    graphArray[j] = 0.0;

  fillCounter = 0;
  firstFill = true;

  u8g2.setFont(u8g2_font_courB08_tr);
  u8g2.setCursor(30, 36);
  if(calibrating)
  {
    zeroVoltage = (float(analogRead(A0)) / 1024.0) * 5.0;
    u8g2.print("Calibrating...");
  }
  else
    u8g2.print("Resetting...");
}

void calculate()
{
  // Get current reading on A0
  currentValue += analogRead(A0);

  if(i >= AVERAGE)
  {
    currentValue /= AVERAGE;
  
    // Get current reading in voltage
    float currentVoltage = (float(currentValue) / 1024.0) * 5.0;
  
    // Calculate voltage into kPa
    float currentKpa = (((currentVoltage / 5.0) - 0.04) / 0.018) - (((zeroVoltage / 5.0) - 0.04) / 0.018);
  
    if(currentKpa > peakKpa)
    {
      peakKpa = currentKpa;
      if(peakKpa > (50.0 / scale))
      {
        int k = scale;
  
        if(peakKpa >= 25.0)
          scale = 1;
        else if(peakKpa >= 10.0)
          scale = 2;
        else if(peakKpa >= 5.0)
          scale = 5;
  
        for(int j = 0; j < PIXELCOUNT; j++)
          graphArray[j] = graphArray[j] / k;
      }
    }

    // Go from right to left for the first fill or after a reset/calibration
    if(firstFill)
    {
      if(fillCounter < PIXELCOUNT)
      {  
        graphArray[fillCounter] = currentKpa * scale;
        fillCounter++;
      }
      else
      {
        fillCounter = 0;
        firstFill = false;
      }
    }
    else
    {
      for(int j = 0; j < (PIXELCOUNT - 1); j++)
        graphArray[j] = graphArray[j + 1];
  
      graphArray[PIXELCOUNT - 1] = currentKpa * scale;
    }

    currentValue = 0;
    i = 1;
  }
  else
    i++;
}
