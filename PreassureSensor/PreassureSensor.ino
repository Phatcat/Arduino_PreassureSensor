#include <U8g2lib.h>

// Define the number of samples to average out
#define AVERAGE 32

// Define the number of pixels on the screen to use for the graph (x-axis)
#define PIXELCOUNT 114

// Set and use the correct mode for the display (ST7920, 128x64, SPI)
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);

// Create array for graphical display of sensor value in graph form and nullify all entries
float graphArray[PIXELCOUNT] = {0.0};

int zeroPixelY = 52;
uint8_t scale = 10;

unsigned int currentValue = 0;
uint8_t i = 1;

float zeroVoltage = 0.0;
float peakKpa = 0.0;
bool overLoad = false;

bool firstFill = true;
uint8_t fillCounter = 0;

bool holdGraph = false;
bool autoScale = true;

int minDisplayValue = 0;
bool outOfDisplayRange = false;

bool printGraph = false;

void setup()
{
  // Calibrate
  pinMode(2, INPUT_PULLUP);
  // Reset
  pinMode(3, INPUT_PULLUP);
  // Hold/Go
  pinMode(4, INPUT_PULLUP);
  // Scale
  pinMode(5, INPUT_PULLUP);
  // Display range up
  pinMode(6, INPUT_PULLUP);
  // Display range down
  pinMode(7, INPUT_PULLUP);
  // Print graph
  pinMode(12, INPUT_PULLUP);

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
    drawBase();

    // Hold / Go
    if(digitalRead(4) == LOW)
    {
      // Hault the program to make sure the state only switches once per button press
      do { ; } while ( digitalRead(4) == LOW );

      // Switch hold state on button release
      holdGraph = !holdGraph;
    }

    // Print Graph
    if(digitalRead(12) == LOW)
    {
      // Hault the program to make sure the state only switches once per button press
      do { ; } while ( digitalRead(12) == LOW );

      // Switch print state on button release
      printGraph = !printGraph;
    }

    // Switch scale
    if(digitalRead(5) == LOW)
    {
      if(autoScale)
        autoScale = false;
      else if(scale > 1)
        switchScale();
      else
      {
        scale = 10;
        autoScale = true;
      }

      // Hault the program to make sure the state only switches once per button press
      do { ; } while ( digitalRead(5) == LOW );
    }

    // Display range
    if(digitalRead(6) == LOW)
    {
      if((minDisplayValue + (50 / scale)) < 50)
      minDisplayValue += 5;

      // Hault the program to make sure the state only switches once per button press
      do { ; } while ( digitalRead(6) == LOW );
    }
    else if(digitalRead(7) == LOW)
    {
      if(minDisplayValue > -50)
        minDisplayValue -= 5;

      // Hault the program to make sure the state only switches once per button press
      do { ; } while ( digitalRead(7) == LOW );
    }

    // Calibration / reset
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

void drawBase()
{
/*********** Draw border frames *************/
  u8g2.drawLine(13, 0, 13, 56);
  u8g2.drawLine(13, 56, 127, 56);
/******* Draw reference pixel points ********/
  u8g2.drawPixel(12, 2);
  u8g2.drawPixel(12, 12);
  u8g2.drawPixel(12, 22);
  u8g2.drawPixel(12, 32);
  u8g2.drawPixel(12, 42);
  u8g2.drawPixel(12, 52);
/********************************************/

  u8g2.setFont(u8g2_font_blipfest_07_tr);

/************ Reference values **************/
  for(uint8_t j = 0; j < 6; j++)
  {
    int refValue = minDisplayValue;
    if(j > 0)
      refValue += (j * 10) / scale;

    uint8_t k = 0;

    if(refValue >= 20)
      k = 4;
    else if(refValue >= 10 || refValue == -1)
      k = 5;
    else if(refValue >= 2 || refValue == 0)
      k = 8;
    else if(refValue == 1)
      k = 9;
    else if(refValue <= -20)
      k = 0;
    else if(refValue <= -10)
      k = 1;
    else if(refValue <= 2)
      k = 4;

    u8g2.setCursor(k, 55 - (10 * j));
    u8g2.print(refValue);
  }
/********************************************/

  if(holdGraph) // ⏸
  {
    u8g2.drawLine(2, 59, 2, 62);
    u8g2.drawLine(4, 59, 4, 62);
  }
  else // ⏵
  {
    u8g2.drawLine(2, 58, 2, 62);
    u8g2.drawLine(3, 59, 3, 61);
    u8g2.drawPixel(4, 60);
  }

  if(printGraph)
  {
    u8g2.setCursor(8, 63);
    u8g2.print("P");
  }

  u8g2.setCursor(15, 63);
  u8g2.print(scale);
  u8g2.print(":1");

  if(autoScale)
  {
    u8g2.setCursor(32, 63);
    u8g2.print("AUTO");
  }

  if(outOfDisplayRange)
  {
    u8g2.setCursor(52, 63);
    u8g2.print("OODR");
  }

  // Set font fitting to the bottom frame
  // u8g2.setFont(u8g2_font_courB08_tr);

  // Fill in text and peak value
  u8g2.setCursor(75, 63);
  u8g2.print("PEAK: ");
    
  if(overLoad)
    u8g2.print("OVERLOAD");
  else
    u8g2.print(peakKpa, 6);
}

void drawGraph()
{
  outOfDisplayRange = false;

  int maxDisplayValue = minDisplayValue + (50 / scale);
  
  if(maxDisplayValue <= 0)
    zeroPixelY = 2 + maxDisplayValue * scale;
  else
    zeroPixelY = 52 + minDisplayValue * scale;
  
  // Draw the graph from the array with a 0-point at zeroPixelY and an absolute leftmost x-point of 14 (horizontal border line + 1)
  for(uint8_t j = 0; j < (firstFill ? fillCounter : PIXELCOUNT); j++)
  {
    if(graphArray[j] > maxDisplayValue)
    {
      u8g2.drawPixel(14 + j, 0);
      outOfDisplayRange = true;
    }
    else if(graphArray[j] < (minDisplayValue - (2 / scale)))
    {
      u8g2.drawPixel(14 + j, 55);
      outOfDisplayRange = true;
    }
    else
    {
      uint8_t displayValue = zeroPixelY - round(graphArray[j] * scale);
/***** Continuous line graph ****************/
      u8g2.drawPixel(14 + j, displayValue);
        if(j > 0)
      u8g2.drawLine(13 + j, zeroPixelY - round(graphArray[j - 1] * scale), 14 + j, displayValue);
/********************************************/
    }
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
  overLoad = false;

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

    if(printGraph)
      Serial.println(currentKpa);

    if(autoScale && (currentKpa > (50 / scale)))
      switchScale();

    if(currentKpa > peakKpa)
    {
      if(currentKpa <= 50.0)
        peakKpa = currentKpa;
      else
        overLoad = true;
    }
    else if((currentKpa < minDisplayValue) && autoScale)
    {
      if(minDisplayValue <= 0)
        switchScale();
      else
        minDisplayValue -= 5;
    }
    else if((currentKpa > (minDisplayValue + (50 / scale))) && autoScale)
      minDisplayValue += 5;

    // Go from right to left for the first fill or after a reset/calibration
    if(firstFill)
    {
      if(fillCounter < PIXELCOUNT)
      {  
        graphArray[fillCounter] = currentKpa;
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
      for(uint8_t j = 0; j < (PIXELCOUNT - 1); j++)
        graphArray[j] = graphArray[j + 1];
  
      graphArray[PIXELCOUNT - 1] = currentKpa;
    }

    currentValue = 0;
    i = 1;
  }
  else
    i++;
}

void switchScale()
{
  if(scale == 10)
    scale = 5;
  else if(scale == 5)
    scale = 2;
  else if(scale == 2)
    scale = 1;
}
