
/*______Import Libraries_______*/
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
/*______End of Libraries_______*/


/*__Select your hardware version__*/

// select one version and deselect the other versions
//#define AZ_TOUCH_ESP            // AZ-Touch ESP
#define AZ_TOUCH_MOD_SMALL_TFT  // AZ-Touch MOD with 2.4 inch TFT
// #define AZ_TOUCH_MOD_BIG_TFT    // AZ-Touch MOD with 2.8 inch TFT

/*____End of hardware selection___*/


/*__Pin definitions for the ESP32__*/
#define TFT_CS   5
#define TFT_DC   4
#define TFT_LED  15
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  22
#define TFT_MISO 19

#define AES_Port 32
#define Kill_Port 33

#define TOUCH_CS 14
#ifdef AZ_TOUCH_ESP
// AZ-Touch ESP
#define TOUCH_IRQ 2
#else
// AZ-Touch MOD
#define TOUCH_IRQ 27
#endif
/*_______End of definitions______*/




/*____Calibrate Touchscreen_____*/
#define MINPRESSURE 10      // minimum required force for touch event
#define TS_MINX 370
#define TS_MINY 470
#define TS_MAXX 3700
#define TS_MAXY 3600
/*______End of Calibration______*/


//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
//XPT2046_Touchscreen touch(TOUCH_CS);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

/*____definition 1Wire and A/D-Converter_____*/
Adafruit_ADS1115 ads1115;
#define I2C_SDA 26
#define I2C_SCL 25

/*____End of definition 1Wire and A/D-Converter_____*/

TwoWire I2CADS = TwoWire(0);
bool status;

/*___________ Setup of times and tensions________________*/

String symbol[4][3] = {
  { "Volt act.", "Min. left" },
  { "2,00 h", "3,00 h" },
  { "3,50 h", "4,00 h" },
  { "12,9 V", "12,8 V" }
};

float time1 = 2;
float time2 = 3;
float time3 = 3.5;
float time4 = 4;

float tension1 = 12.9;
float tension2 = 12.8;

float tension_min_limit = 12.7;
float timelimit = 5;

/*___________ End of setup of times and tensions________________*/

int X, Y;
float execution_time;
double minutes_left, tension_act, tension_limit;
unsigned long time_now, start_time, time_actual, time_last;
bool limit_type_tension; // True: tension as limit not time
bool Touch_pressed = false;
int case_number;
TS_Point p;

int freq = 2000;
int buzzChannel = 0;
int resolution = 8;

void setup() {
  execution_time = timelimit;
  hours_to_millis();

  pinMode(AES_Port, OUTPUT);
  digitalWrite(AES_Port, LOW);

  pinMode(Kill_Port, OUTPUT);
  digitalWrite(Kill_Port, LOW);

  tension_limit = tension_min_limit;
  minutes_left = 0;
  time_now = millis();
  start_time = millis();
  limit_type_tension = true;


  Serial.begin(115200); //Use serial monitor for debugging

  // Setup A/D-Converter
  Serial.print("Execution time:"); Serial.println(execution_time);
  Serial.println(F("ADS1115 test"));
  I2CADS.begin(I2C_SDA, I2C_SCL, 100000);
  Serial.println("Getting Single-Ended reading from AIN0 (P) and AIN1 (N)");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV)");
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = ads1115.begin(0x48, &I2CADS);  // Initialize ads1115 at the default address 0x48
  ads1115.setGain(GAIN_TWOTHIRDS);
  if (!status) {
    Serial.println("Could not find a valid ads1115 sensor, check wiring!");
    while (1);
  }

  read_voltage();

  // Setup Display
  pinMode(TFT_LED, OUTPUT); // define as output for backlight control
  Serial.println("Init TFT and Touch...");
  tft.begin();
  touch.begin();
  Serial.print("tftx ="); Serial.print(tft.width()); Serial.print(" tfty ="); Serial.println(tft.height());
  tft.fillScreen(ILI9341_BLACK);
  IntroScreen();

  delay(1500);
  Serial.println("drawButtons");
  draw_BoxNButtons();

  //sound configuration
  ledcSetup(buzzChannel, freq, resolution);
  ledcAttachPin(21, buzzChannel);
}


void loop() {
  CheckState();
  // check touch screen for new events
  if (Touch_Event() == true) {
    X = p.y; Y = p.x;
    Touch_pressed = true;

  } else {
    Touch_pressed = false;
  }

  // if touch is pressed detect pressed buttons
  if (Touch_pressed == true) {

    DetectButtons();

    if (limit_type_tension == true) {

      switch (case_number) {
        case 1:
          draw_BoxNButtons();
          tft.fillRect  (0, 241, 120, 79, ILI9341_RED);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_WHITE);
          tft.setCursor(8, 270);
          tft.println(symbol[3][0]);
          Serial.println(symbol[3][0]);

          ledcWriteTone(buzzChannel, 2000);
          delay(500);
          ledcWriteTone(buzzChannel, 0);
          delay(50);
          break;

        case 2:
          //Draw Voltage Input Result
          draw_BoxNButtons();
          tft.fillRect  (121, 241, 119, 79, ILI9341_RED);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_WHITE);
          tft.setCursor(128, 270);
          tft.println(symbol[3][1]);
          Serial.println(symbol[3][1]);

          ledcWriteTone(buzzChannel, 2000);
          delay(500);
          ledcWriteTone(buzzChannel, 0);
          delay(50);
          break;

        default:
          // if nothing else matches, do the default
          // default is optional
          break;
      }
    }
    else {
      switch (case_number) {
        case 1:
          draw_BoxNButtons();
          tft.fillRect  (0, 81, 120, 79, ILI9341_RED);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_WHITE);
          tft.setCursor(8, 110);
          tft.println(symbol[1][0]);
          Serial.println(symbol[1][0]);

          ledcWriteTone(buzzChannel, 4000);
          delay(500);
          ledcWriteTone(buzzChannel, 0);
          delay(50);

          break;
        case 2:
          //Draw Voltage Input Result
          draw_BoxNButtons();
          tft.fillRect  (121, 81, 119, 79, ILI9341_RED);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_WHITE);
          tft.setCursor(128, 110);
          tft.println(symbol[1][1]);
          Serial.println(symbol[1][1]);

          ledcWriteTone(buzzChannel, 4000);
          delay(500);
          ledcWriteTone(buzzChannel, 0);
          delay(50);

          break;

        case 3:
          draw_BoxNButtons();
          tft.fillRect  (0, 161, 120, 79, ILI9341_RED);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_WHITE);
          tft.setCursor(8, 190);
          tft.println(symbol[2][0]);
          Serial.println(symbol[2][0]);

          ledcWriteTone(buzzChannel, 4000);
          delay(500);
          ledcWriteTone(buzzChannel, 0);
          delay(50);

          break;

        case 4:
          //Draw Voltage Input Result
          draw_BoxNButtons();
          tft.fillRect  (121, 161, 119, 79, ILI9341_RED);
          tft.setTextSize(3);
          tft.setTextColor(ILI9341_WHITE);
          tft.setCursor(128, 190);
          tft.println(symbol[2][1]);
          Serial.println(symbol[2][1]);

          ledcWriteTone(buzzChannel, 4000);
          delay(500);
          ledcWriteTone(buzzChannel, 0);
          delay(50);

          break;
        default:
          // if nothing else matches, do the default
          // default is optional
          break;
      }
    }
  }
}

/********************************************************************//**
   @brief     detects a touch event and converts touch data
   @param[in] None
   @return    boolean (true = touch pressed, false = touch unpressed)
 *********************************************************************/
bool Touch_Event() {
  p = touch.getPoint();
  delay(1);
#ifdef AZ_TOUCH_MOD_BIG_TFT
  // 2.8 inch TFT with yellow header
  p.x = map(p.x, TS_MINX, TS_MAXX, 320, 0);
  p.y = map(p.y, TS_MINY, TS_MAXY, 240, 0);
#else
  // 2.4 inch TFT with black header
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, 320);
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, 240);
#endif
  if (p.z > MINPRESSURE) return true;
  return false;
}


/********************************************************************//**
   @brief     detecting pressed buttons with the given touchscreen values
   @param[in] None
   @return    None
 *********************************************************************/
void DetectButtons()
{
  if (X > 120) //Detecting Buttons on Column 1
  {
    if (Y > 240) //If cancel Button is pressed
    { Serial.println ("Button 13 V");

      limit_type_tension = true;
      tension_limit = tension1;
      execution_time = timelimit;
      hours_to_millis();
      start_time = millis();
      case_number = 1;


    }

    if (Y > 160 && Y < 240) //If Button 1 is pressed
    { Serial.println ("Button 3,5 h");

      limit_type_tension = false;
      execution_time = time3;
      hours_to_millis();
      start_time = millis();
      case_number = 3;


    }

    if (Y > 80 && Y < 160) //If Button 4 is pressed
    { Serial.println ("Button 2 h");

      limit_type_tension = false;
      execution_time = time1;
      hours_to_millis();
      start_time = millis();
      case_number = 1;


    }

  }

  if (X < 120) //Detecting Buttons on Column 2
  {
    if (Y > 240)
    { Serial.println ("Button 12,9 V"); //Button 0 is Pressed

      limit_type_tension = true;
      tension_limit = tension2;
      execution_time = timelimit;
      hours_to_millis();
      start_time = millis();
      case_number = 2;


    }

    if (Y > 160 && Y < 240)
    { Serial.println ("Button 4 h");

      limit_type_tension = false;
      execution_time = time4;
      hours_to_millis();
      start_time = millis();
      case_number = 4;


    }

    if (Y > 80 && Y < 160)
    { Serial.println ("Button 3 h");

      limit_type_tension = false;
      execution_time = time2;
      hours_to_millis();
      start_time = millis();
      case_number = 2;


    }
  }
}


/********************************************************************//**
   @brief     draws the keypad
   @param[in] None
   @return    None
 *********************************************************************/
void draw_BoxNButtons()
{

  //clear screen black
  tft.fillRect(0, 0, 240, 320, ILI9341_BLACK);
  tft.setFont(0);

  //Draw the Actual Data Box
  tft.fillRect(0, 0, 240, 80, ILI9341_GREEN);

  //Draw Time Input
  tft.fillRect  (0, 80, 240, 160, ILI9341_BLUE);

  //Draw Voltage Input
  tft.fillRect  (0, 240, 240, 80, ILI9341_MAGENTA);

  //Draw Horizontal Lines
  for (int h = 80; h <= 320; h += 80)
    tft.drawFastHLine(0, h, 240, ILI9341_WHITE);

  //Draw Vertical Lines
  for (int v = 120; v <= 240; v += 120)
    tft.drawFastVLine(v, 0, 320, ILI9341_WHITE);

  //Display keypad lables
  tft.setCursor (8, 3); //Voltage
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.println(symbol[0][0]);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor (128, 3); //Time left
  tft.println(symbol[0][1]);

  for (int j = 1; j < 4; j++) {
    for (int i = 0; i < 2; i++) {
      tft.setCursor(8 + (120 * i), 30 + (80 * j));
      //if ((j==3) && (i==2)) tft.setCursor(24 + (80*i), 100 + (80*j));
      tft.setTextSize(3);
      tft.setTextColor(ILI9341_BLACK);
      tft.println(symbol[j][i]);
    }
  }
}


/********************************************************************//**
   @brief     Write actual parameters
   @param[in] None
   @return    None
 *********************************************************************/
void CheckState() {


  Serial.print("Execution time:"); Serial.print(execution_time); Serial.print(" Minutes left:"); Serial.println(minutes_left);
  read_voltage();

  long minutes_now, minutes_execution, minutes_start;
  time_actual = millis();
  time_now = millis();

  minutes_now  = time_now / 1000 / 60;
  minutes_execution  = execution_time / 1000 / 60;
  minutes_start = start_time / 1000 / 60;
  minutes_left = minutes_execution - (minutes_now - minutes_start);

  if (tension_act < tension_min_limit) {                          // Absolut no power left > Shut off
    digitalWrite(AES_Port, LOW);
    digitalWrite(Kill_Port, HIGH);
  }

  if (minutes_left > 0) {                             // Still time? > Cool on
    digitalWrite(AES_Port, HIGH);
  }

  if (tension_act > tension_limit) {                   // Still power? > Cool on
    digitalWrite(AES_Port, HIGH);
  }


  if (minutes_left <= 0) {                            // No time left > Shut off
    digitalWrite(AES_Port, LOW);
    digitalWrite(Kill_Port, HIGH);

  }

  if (tension_act < tension_limit) {                            // No power left > Shut off
    digitalWrite(AES_Port, LOW);

    digitalWrite(Kill_Port, HIGH);

  }

  if (time_actual - time_last >= 500) {
    // Print actual Voltage
    tft.fillRect  (0, 34, 119, 40, ILI9341_GREEN);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(20, 35);
    tft.println(tension_act);

    // Print time left
    tft.fillRect  (121, 34, 119, 40, ILI9341_GREEN);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(160, 35);// (160, 35)
    tft.println(minutes_left, 0);

    time_last = millis();
  }
}

/********************************************************************//**
   @brief     read_voltage
   @param[in] None
   @return    None
 *********************************************************************/
void read_voltage() {
  float results, adc0;
  float volts0;
  results = 0;
  for (int i = 1; i <= 5; i++) {
    adc0 = ads1115.readADC_SingleEnded(0);

    results = results + (adc0 * 0.001073);
  }
  results = results / 5;
  tension_act = results;
  Serial.print("Differential: "); Serial.print(adc0); Serial.print("("); Serial.print(results); Serial.print("mV) "); Serial.print("V: "); Serial.println(tension_act);
}

/********************************************************************//**
   @brief     hours in milliseconds
   @param[in] None
   @return    None
 *********************************************************************/
void hours_to_millis() {
  execution_time = execution_time * 60 * 60 * 1000;
}


/********************************************************************//**
   @brief     shows the intro screen in setup procedure
   @param[in] None
   @return    None
 *********************************************************************/
void IntroScreen()
{
  //Draw the Result Box
  tft.fillRect(0, 0, 240, 320, ILI9341_WHITE);
  tft.setTextSize(0);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSansBold9pt7b);

  tft.setCursor(25, 215);
  tft.println("Battery Guard");
}
