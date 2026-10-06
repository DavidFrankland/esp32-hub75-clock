#include <Arduino.h>
#include <xtensa/core-macros.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <time.h>
#include <WiFi.h>

#include "main.h"
#include "wifi-credentials.h"

// HUB75E pinout
// R1 | G1
// B1 | GND
// R2 | G2
// B2 | E
//  A | B
//  C | D
// CLK| LAT
// OE | GND

/* Default library pin configuration for the reference. You can redefine only ones you need later on object creation
#define R1 25
#define G1 26
#define BL1 27
#define R2 14
#define G2 12
#define BL2 13
#define CH_A 23
#define CH_B 19
#define CH_C 5
#define CH_D 17
#define CH_E -1 // assign to any available pin if using panels with 1/32 scan
#define CLK 16
#define LAT 4
#define OE 15
*/

// Configure for your panel(s) as appropriate!
#define PIN_E 18
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64 // Panel height of 64 will required PIN_E to be defined.
#define PANELS_NUMBER 1 // Number of chained panels, if just a single panel, obviously set to 1
#define PANE_WIDTH PANEL_WIDTH *PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT
#define NUM_LEDS PANE_WIDTH *PANE_HEIGHT

MatrixPanel_I2S_DMA *matrix = nullptr;

const uint16_t centreX = PANE_WIDTH / 2 - 1;
const uint16_t centreY = PANE_HEIGHT / 2 - 1;

const uint16_t black = matrix->color565(0, 0, 0);
const uint16_t red = matrix->color565(255, 0, 0);
const uint16_t green = matrix->color565(0, 255, 0);
const uint16_t yellow = matrix->color565(255, 255, 0);
const uint16_t blue = matrix->color565(0, 0, 255);
const uint16_t magenta = matrix->color565(255, 0, 255);
const uint16_t cyan = matrix->color565(0, 255, 255);
const uint16_t white = matrix->color565(255, 255, 255);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// serial debug port baud rate
const unsigned int BAUD_RATE = 115200;

void setup()
{
  Serial.begin(BAUD_RATE);

  // redefine pins if required
  // HUB75_I2S_CFG::i2s_pins _pins={R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
  HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, PANELS_NUMBER);
  mxconfig.gpio.e = PIN_E;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;
  mxconfig.clkphase = false;

  // use a single buffer for startup messages
  mxconfig.double_buff = false;
  matrix = new MatrixPanel_I2S_DMA(mxconfig);
  matrix->begin();
  matrix->setBrightness8(100);

  // set the time using NTP
  setTime();

  // wait so we can read the startup messages
  delay(1000);

  // use a double buffer for clock display
  mxconfig.double_buff = true;
  matrix = new MatrixPanel_I2S_DMA(mxconfig);
  matrix->begin();
  matrix->setBrightness8(100);
}

void loop()
{
  // get time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("getLocalTime() failed");
    delay(1000);
    return;
  }

  // draw time
  matrix->clearScreen();
  clockFaceOuter();
  clockFaceTicks();

  // buffer for day, date, month strings
  const uint8_t bufSize = 20;
  char buf[bufSize];

  // day of week text
  strftime(buf, bufSize, "%A", &timeinfo);
  centreText(buf, centreY - 8, blue);

  // month name text
  strftime(buf, bufSize, "%B", &timeinfo);
  centreText(buf, centreY + 10, blue);

  // date number text
  snprintf(buf, bufSize, "%d", timeinfo.tm_mday);
  centreText(buf, centreY + 19, blue);

  // hands
  uint8_t hour = timeinfo.tm_hour % 12;
  uint8_t min = timeinfo.tm_min;
  uint8_t sec = timeinfo.tm_sec;
  clockHand(0, 28, sec / 60.0, green);
  clockHand(0, 28, (min + sec / 60.0) / 60.0, red);
  clockHand(0, 16, (hour + min / 60.0) / 12.0, red);

  // centre circle
  clockFaceInner();

  matrix->flipDMABuffer();
  delay(10);
}

void centreText(char *myText, uint8_t y, uint16_t colour)
{
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;
  matrix->getTextBounds(myText, 0, 0, &x1, &y1, &w, &h);
  matrix->setCursor(32 - w / 2, y - h / 2);
  matrix->setTextColor(colour);
  matrix->print(myText);
}

void setTime()
{
  matrix->setTextColor(white);
  matrix->println("Connecting");
  matrix->setTextColor(yellow);
  matrix->println(WIFI_SSID);
  const char *spinnerChars = "|/-\\";
  uint8_t spinnerCharsLen = strlen(spinnerChars);
  uint8_t i = 0;
  int16_t x = matrix->getCursorX();
  int16_t y = matrix->getCursorY();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  matrix->setTextColor(white, black);
  while (WiFi.status() != WL_CONNECTED)
  {
    matrix->print(spinnerChars[i]);
    matrix->setCursor(x, y);
    i = (i + 1) % spinnerCharsLen;
    delay(250);
  }
  matrix->setTextColor(green, black);
  matrix->println("Connected");

  // init and get the time
  matrix->setTextColor(white);
  matrix->println("NTP");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // this is necessary, for some reason
  printLocalTime();

  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void printLocalTime()
{
  struct tm timeinfo;
  int16_t x = matrix->getCursorX();
  int16_t y = matrix->getCursorY();
  while (!getLocalTime(&timeinfo))
  {
    matrix->setTextColor(red, black);
    matrix->println("Time fail");
    matrix->setCursor(x, y);
    delay(1000);
    matrix->println("         ");
    matrix->setCursor(x, y);
  }
  matrix->setTextColor(green);
  matrix->println("Success");

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char myString[50];
  strftime(myString, 50, "%d %b", &timeinfo);
  Serial.println(myString);
}

void clockFaceOuter()
{
  matrix->fillCircle(centreX, centreY, 31, white);
  matrix->fillCircle(centreX, centreY, 29, black);
}

void clockFaceInner()
{
  matrix->fillCircle(centreX, centreY, 2, white);
}

void clockFaceTicks()
{
  for (uint8_t i = 0; i < 12; i++)
  {
    clockHand(28, 28, i / 12.0, white);
  }
}

void clockHand(uint16_t innerRadius, uint16_t outerRadius, float angle, uint16_t colour)
{
  float theta = angle * TWO_PI;
  float sin_theta = sin(theta);
  float cos_theta = cos(theta);
  uint16_t x1 = centreX + innerRadius * sin_theta + 0.5;
  uint16_t y1 = centreY - innerRadius * cos_theta + 0.5;
  uint16_t x2 = centreX + outerRadius * sin_theta + 0.5;
  uint16_t y2 = centreY - outerRadius * cos_theta + 0.5;
  matrix->drawLine(x1, y1, x2, y2, colour);
}
