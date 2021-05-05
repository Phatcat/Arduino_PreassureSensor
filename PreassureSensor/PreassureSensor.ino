#include "Adafruit_GFX.h"// Hardware-specific library
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#include "TouchScreen_kbv.h"

// Define the touch pins
#define XP 8
#define XM A2
#define YM 9
#define YP A3

// Touchscreen
TouchScreen_kbv ts(XP, YP, XM, YM, 300);

// Touchscreen point
TSPoint_kbv tp;

// Assign human-readable names to the 16-bit color values we´ll utilize:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0xC618

// Define the number of samples to average out
#define AVERAGE 32

// Define the number of pixels on the screen to use for the graph (x-axis)
#define PIXELCOUNT 296

// Create array for kPa value and graphical display value respectively and nullify all entries
float graphArray[PIXELCOUNT] = {0.0};
int displayArray[PIXELCOUNT] = {0};
uint16_t fillCounter = 0;

uint8_t zeroPixelY = 107;
uint8_t scale = 10;

unsigned int currentValue = 0;
uint16_t i = 1;

float zeroVoltage = 0.0;
float peakKpa = 0.0;
bool overLoad = false;

bool holdGraph = false;
bool autoScale = true;

int minDisplayValue = -5;
bool outOfDisplayRange = false;

bool printGraph = false;

void setup()
{
  Serial.begin(9600);
  tft.begin(tft.readID());

  // Set rotation (0 - 3, portrait, landscape, rev. portait, rev. landscape)
  tft.setRotation(1);

  tft.setTextSize(1);

  tft.fillScreen(BLACK);

  // Initial calibration of sensor input
  drawConfig(true);

  // Draw borders and other graphics that don´t need updating
  drawBase();

  refreshAuto(true);
  refreshHold(false);

  refreshScale();
}

void loop()
{
  // Touch control 
  if(ISPRESSED())
  {
    int x = tp.x;
    int y = tp.y;

    // Touch is engadged and at the correct X-axis values below the graph
    if(tp.x < 800)
    {
      while(ISPRESSED())
      {
        readResistiveTouch();
        x = tp.x;
        delay(100);
        readResistiveTouch();
  
        // Slide upwards
        if((x >= (tp.x + 20)) && (minDisplayValue > -50))
        {
          x = tp.x;
          delay(100);
          readResistiveTouch();
          if(x >= (tp.x + 20))
          {
            minDisplayValue -= 5;
  
            // Reset display
            refreshScale();
          }
        }
        // Slide downwards
        else if((x <= (tp.x - 20)) && ((minDisplayValue + (100 / scale)) < 50))
        {
          x = tp.x;
          delay(100);
          readResistiveTouch();
          if(x <= (tp.x - 20))
          {
            minDisplayValue += 5;
  
            // Reset display
            refreshScale();
          }
        }
      }
    }
    else if((tp.x >= 820) && (tp.y < 575))
    {
      bool doRefreshScale = false;
      bool doRefreshGraph = false;
      if(tp.y < 130)                // Toggle Hold / Running graph
        refreshHold(!holdGraph);
      else if(tp.y < 215)           // Switch scale
        switchScale();
      else if(tp.y < 350)           // Toggle Auto / Manual scaling
        refreshAuto(!autoScale);
      else if(tp.y < 410)           // Reset (Nullify graph and peak)
      {
        drawConfig(false);
        doRefreshScale = true;
      }
      else if(tp.y < 480)           // Calibrate (Set current read as 0 and nullify graph and peak)
      {
        drawConfig(true);
        doRefreshScale = true;
      }
      else if(tp.y < 575)           // Toggle serial printing of the graph
      {
        printGraph = !printGraph;

        tft.setCursor(130, 136);

        if(printGraph)
          tft.print("Serial Print ON");
        else
          tft.print("Serial Print OFF");

        doRefreshGraph = true;
      }

      // Wait until touch is disengadged
      do { ; } while ( ISPRESSED() );

      if(doRefreshScale)
        refreshScale();
      else if(doRefreshGraph)
        refreshGraph();
    }
  }
  else if(!holdGraph)
  {
    // Get current reading on A6
    currentValue += analogRead(A6);

    if(i >= AVERAGE)
    {
      calculate();
      drawGraph();
    }
    else
      i++;
  }
}

void calculate()
{
  currentValue /= AVERAGE;

  // Get current reading in voltage
  float currentVoltage = (float(currentValue) / 1024.0) * 5.0;
  
  // Calculate voltage into kPa
  float currentKpa = (((currentVoltage / 5.0) - 0.04) / 0.018) - (((zeroVoltage / 5.0) - 0.04) / 0.018);

  if(printGraph)
    Serial.println(currentKpa);

  if(currentKpa > peakKpa)
  {
    if(currentKpa <= 50.0)
      peakKpa = currentKpa;
    else
      overLoad = true;

    refreshPeak();
  }

  if(fillCounter >= (PIXELCOUNT - 1))
  {
    for(uint16_t j = 0; j < (PIXELCOUNT - 100); j++)
    {
      graphArray[j] = graphArray[j + 99];
      displayArray[j] = displayArray[j + 99];
    }
    for(uint16_t j = (PIXELCOUNT - 100); j < PIXELCOUNT; j++)
    {
      graphArray[j] = 0.0;
      displayArray[j] = 0;
    }

    // Set the fillCounter to account for the array having shifted values 100 positions to the left
    fillCounter = PIXELCOUNT - 100;

    // Clear the graph before shifting the graph 100 pixels
    refreshGraph();
  }
  else
    fillCounter++;

  graphArray[fillCounter] = currentKpa;
  displayArray[fillCounter] = round(currentKpa * scale * 2);

  currentValue = 0;
  i = 1;
}

void drawBase()
{
/*********** Draw border frames *************/
  tft.drawLine(23, 0, 23, 212, WHITE);
  tft.drawLine(22, 0, 22, 212, WHITE);
  tft.drawLine(22, 211, 319, 211, WHITE);
  tft.drawLine(23, 212, 319, 212, WHITE);

/******* Draw reference pixel points ********/
  tft.drawPixel(20, 7, WHITE);
  tft.drawPixel(20, 27, WHITE);
  tft.drawPixel(20, 47, WHITE);
  tft.drawPixel(20, 67, WHITE);
  tft.drawPixel(20, 87, WHITE);
  tft.drawPixel(20, 107, WHITE);
  tft.drawPixel(20, 127, WHITE);
  tft.drawPixel(20, 147, WHITE);
  tft.drawPixel(20, 167, WHITE);
  tft.drawPixel(20, 187, WHITE);
  tft.drawPixel(20, 207, WHITE);
/********************************************/

  tft.setTextColor(WHITE);

  tft.setCursor(110, 223);
  tft.print("RES");

  tft.setCursor(135, 223);
  tft.print("CAL");

  tft.setCursor(160, 223);
  tft.print("PRINT");
}

void drawGraph()
{
  outOfDisplayRange = false;

  int maxDisplayValue = minDisplayValue + (100 / scale);
  uint8_t displayScale = scale * 2;
  
  if(maxDisplayValue <= 0)
    zeroPixelY = 7 + (maxDisplayValue * displayScale);
  else
    zeroPixelY = 207 + (minDisplayValue * displayScale);

  // Draw the graph from the array with a 0-point at zeroPixelY and an absolute leftmost x-point of 14 (horizontal border line + 1)
  for(uint16_t j = 0; j < fillCounter; j++)
  {
    int displayValue = zeroPixelY - displayArray[j];
    int roundValue = round(graphArray[j]);

    if(roundValue > maxDisplayValue)
    {
      tft.drawPixel(24 + j, 0, RED);
      outOfDisplayRange = true;
    }
    else if(roundValue < minDisplayValue)
    {
      tft.drawPixel(24 + j, 210, RED);
      outOfDisplayRange = true;
    }
    else
    {
/***** Continuous line graph ****************/
      tft.drawPixel(24 + j, displayValue, BLUE);
      if(j > 0)
        tft.drawLine(23 + j, zeroPixelY - displayArray[j - 1], 24 + j, displayValue, BLUE);
/********************************************/
    }
  }

  refreshOODR();

  // Auto-scaling and auto-ranging
  if(outOfDisplayRange && autoScale)
  {
    if(scale > 1)
    {
      switchScale();
      refreshScale();
    }
  }
}

void drawConfig(bool calibrating)
{
  tft.setTextColor(YELLOW);
  tft.setCursor(130, 136);
  if(calibrating)
  {
    zeroVoltage = (float(analogRead(A6)) / 1024.0) * 5.0;
    tft.print("Calibrating...");
  }
  else
    tft.print("Resetting...");

  // Reset peak
  peakKpa = 0.0;
  refreshPeak();

  // Reset scale to 10:1
  scale = 10;

  // Reset range to the interval of -5 to +5
  minDisplayValue = -5;

  // Nullify arrays
  for(uint16_t j = 0; j < PIXELCOUNT; j++)
  {
    graphArray[j] = 0.0;
    displayArray[j] = 0;
  }

  fillCounter = 0;
  overLoad = false;
}

void switchScale()
{
  if(scale == 10)
  {
    scale = 5;
    minDisplayValue *= 2;
  }
  else if(scale == 5)
  {
    scale = 2;
    minDisplayValue *= 2.5;
  }
  else if(scale == 2)
  {
    scale = 1;
    minDisplayValue = -50;
  }
  else if(scale == 1)
  {
    scale = 10;
    minDisplayValue = -5;
  }

  for(uint16_t j = 0; j < PIXELCOUNT; j++)
    if(displayArray[j] != 0)
      displayArray[j] = round(graphArray[j] * scale * 2);

  refreshScale();
}

void refreshScale()
{
  tft.fillRect(0, 0, 20, 212, BLACK);
  tft.fillRect(26, 215, 34, 15, BLACK);

  tft.setTextColor(WHITE);
  tft.setCursor(27, 223);
  tft.print(scale);
  tft.print(":1");

/************ Reference values **************/
  for(uint8_t j = 0; j < 12; j++)
  {
    int refValue = minDisplayValue;
    if(j > 0)
      refValue += (j * 10) / scale;

    uint8_t k = 0;

    if(refValue >= 10)
      k = 4;
    else if(refValue >= 0)
      k = 10;
    else if(refValue <= -10)
      k = 0;
    else if(refValue < 0)
      k = 4;

    tft.setCursor(k, 205 - (20 * j));
    tft.print(refValue);
  }
/********************************************/

  refreshGraph();
}

void refreshHold(bool hold)
{
  tft.fillRect(0, 215, 25, 20, BLACK);

  if(hold) // ⏸
  {
    tft.drawLine(10, 222, 10, 230, YELLOW);
    tft.drawLine(11, 222, 11, 230, YELLOW);
    
    tft.drawLine(14, 222, 14, 230, YELLOW);
    tft.drawLine(15, 222, 15, 230, YELLOW);

    holdGraph = true;
  }
  else
  {
    tft.drawLine(10, 221, 10, 231, GREEN);
    tft.drawLine(11, 222, 11, 230, GREEN);
    tft.drawLine(12, 223, 12, 229, GREEN);
    tft.drawLine(13, 224, 13, 228, GREEN);
    tft.drawLine(14, 225, 14, 227, GREEN);
    tft.drawPixel(15, 226, GREEN);

    holdGraph = false;
  }
}

void refreshAuto(bool autoS)
{
  tft.fillRect(60, 215, 50, 15, BLACK);

  tft.setTextColor(WHITE);
  tft.setCursor(60, 223);
  if(autoS)
  {
    tft.print("AUTO");
    autoScale = true;
  }
  else
  {
    tft.print("MANUAL");
    autoScale = false;
  }
}

void refreshGraph()
{
  tft.fillRect(24, 0, 147, 107, BLACK);
  tft.fillRect(172, 0, 148, 107, BLACK);
  tft.fillRect(24, 108, 147, 103, BLACK);
  tft.fillRect(172, 108, 148, 103, BLACK);

  /************* Draw crosshairs **************/
  tft.drawLine(24, 107, 319, 107, GRAY);
  tft.drawLine(171, 0, 171, 210, GRAY);

  drawGraph();
}

void refreshOODR()
{
  tft.fillRect(195, 215, 30, 15, BLACK);

  if(outOfDisplayRange)
  {
    tft.setTextColor(RED);
    tft.setCursor(198, 223);
    tft.print("OODR");
  }
}

void refreshPeak()
{
  tft.fillRect(230, 215, 89, 15, BLACK);

  // Fill in text and peak value
  tft.setTextColor(BLUE);
  tft.setCursor(230, 223);
  tft.print("PEAK:");

  if(overLoad)
  {
    tft.setTextColor(RED);
    tft.print("OVERLOAD");
  }
  else
    tft.print(peakKpa, 6);
}

bool ISPRESSED(void)
{
    readResistiveTouch();
    return (tp.z > 300);
}

void readResistiveTouch(void)
{
    tp = ts.getPoint();
    pinMode(YP, OUTPUT);      // restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   // because TFT control pins
    digitalWrite(XM, HIGH);
}
