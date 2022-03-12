// --------------------------------------------------
//
// ESP32 Bluetooth App part 2 -> bi-directional cummunication
//
// Code for bi-directional Bluetooth communication demonstration between the ESP32 and mobile phone (with MIT inventor app).
// device used for tests: ESP32-WROOM-32D
//

//
// --------------------------------------------------


/*______Import Libraries_______*/
// this header is needed for Bluetooth Serial -> works ONLY on ESP32
#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
/*______End of Libraries_______*/


#define sent_intervall 2000 //2 sec time between sending

#define AES_Port 19
#define Kill_Port 32
#define pushButton_Port 35
#define green_LED  18
#define red_LED  33
#define blue_LED  23

/*____definition 1Wire and A/D-Converter_____*/
Adafruit_ADS1115 ads1115;
bool status;
/*____End of definition 1Wire and A/D-Converter_____*/

int buttonState = 0;         // variable for reading the pushbutton status

// Parameters for Bluetooth interface and timing
//int incoming;                           // variable to store byte received from phone
unsigned long now;                      // variable to store current "time" using millis() function
unsigned long last_sent;                // variable to store point in time of sending

double tension_lost = 0; //only for simulation

double start_time;
double voltage;                         //actual tension
double tension_limit = 12.9;                   //tension limit for cut-off
double minutes_runtime = 120;                    //actual minutes set
double time_left = 120;                     //actual runtime until cut-off
double actual_minutes = 0;                     //actual runtime in minutes
String AES = "0";
String make_string;
char sent_char[20];
String buff;
String state = "";

// Bluetooth Serial object
BluetoothSerial SerialBT;

void setup() {

  set_LED_colour('g');
  last_sent = millis();
  Serial.begin(115200);
  // Setup A/D-Converter

  Serial.println(F("ADS1115 test"));

  Serial.println("Getting Single-Ended reading from AIN0 (P)");

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = ads1115.begin();
  Serial.println("ADC Range: +/- 4.096V  1 bit = 2mV");
  // ads115.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV (default)

  ads1115.setGain(GAIN_ONE);     // 1x gain   +/- 4.096V  1 bit = 2mV
  // ads1115.setGain(GAIN_TWO);     // 2x gain   +/- 2.048V  1 bit = 1mV
  // ads1115.setGain(GAIN_FOUR);    // 4x gain   +/- 1.024V  1 bit = 0.5mV
  // ads1115.setGain(GAIN_EIGHT);   // 8x gain   +/- 0.512V  1 bit = 0.25mV
  // ads1115.setGain(GAIN_SIXTEEN); // 16x gain  +/- 0.256V  1 bit = 0.125mV

  if (!status) {
    Serial.println("Could not find a valid ads1115 sensor, check wiring!");
    while (1);
  }
  SerialBT.begin("ESP32_Control");        // Name of your Bluetooth interface -> will show up on your phone
  pinMode(green_LED, OUTPUT);    // sets the LED pins as output
  pinMode(red_LED, OUTPUT);    // sets the LED pins as output
  pinMode(blue_LED, OUTPUT);    // sets the LED pins as outputgreen_LED

  pinMode(AES_Port, OUTPUT);
  pinMode(Kill_Port, OUTPUT);
  pinMode(pushButton_Port, INPUT);

  digitalWrite(AES_Port, LOW);
  digitalWrite(Kill_Port, LOW);

  pinMode(pushButton_Port, INPUT);
}

void time_to_go() {
  actual_minutes = now / 60000;
  time_left = minutes_runtime - (actual_minutes - start_time);
}


/********************************************************************//**
   @brief     read_voltage
   @param[in] None
   @return    None
 *********************************************************************/
void read_voltage() {
  double results, adc0;
  //double volts0;
  results = 0;
  for (int i = 1; i <= 5; i++) {
    adc0 = ads1115.readADC_SingleEnded(0);
    adc0 = adc0 * 0.0009926;
    adc0 = adc0 - 7.369;
    results = results + adc0;
  }
  results = results / 5;
  voltage = results;
  //Serial.print("Differential: "); Serial.print(adc0); Serial.print("("); Serial.print(results); Serial.print("mV) "); Serial.print("V: "); Serial.println(voltage);
}

void check_state() {

  /* ##############################
      tension
    ###############################
    // simulate Voltage
    double factor = random(0, 100); // create random number
    tension_lost = tension_lost + (factor / 500);
    voltage = 15 -  tension_lost;
    // end simulate
  */

  read_voltage();
  if (voltage <= tension_limit) {
    AES = '0';
    Serial.print("Tension low. Tension_limit: "); Serial.print(tension_limit); Serial.print(" Voltage: "); Serial.println(voltage);
    Serial.println(" ");
    digitalWrite(AES_Port, LOW);
    //digitalWrite(Kill_Port, HIGH);
  }

  /* ##############################
      time
    ###############################*/
  time_to_go();
  if (time_left <= 0) {
    AES = '0';
    Serial.print("Time out. Time_left: "); Serial.println(time_left);
    Serial.println(" ");
    digitalWrite(AES_Port, LOW);
    digitalWrite(Kill_Port, HIGH);
  }
}

/*###########################################
   Set LED-status
   g: green
   r: red
   b: blue
   o: off
  ############################################*/

void set_LED_colour(char LED_Colour) {
  switch (LED_Colour) {
    case 'g':
      digitalWrite(green_LED, HIGH);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, LOW);
      break;
    case 'r':
      Serial.println("LED rot");
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, HIGH);
      digitalWrite(blue_LED, LOW);
      break;
    case 'b':
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, HIGH);
      break;
    case 'o':
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, LOW);
      break;
    default:
      // statements
      break;
  }
}

void read_status () {
  make_string = "";
  buff = "";
  buff = String(voltage, 2);
  make_string = make_string + buff + "/";
  buff = String(tension_limit, 1);
  make_string = make_string + buff + "/";
  buff = String(time_left, 0);
  make_string = make_string + buff + "/" + AES + "/";;

  make_string.toCharArray(sent_char, (make_string.length()) + 1);
}

void send_BT() {
  now = millis();                       // Store current time

  if (now > (last_sent + sent_intervall)) {
    check_state();
    read_status();
    SerialBT.println(sent_char);
    //set_LED_colour ('b');
    delay(500);
    Serial.print("Volt: "); Serial.println(sent_char);
    last_sent = millis();

  }
}

void receive_BT() {
  char label = ' ';

  if (SerialBT.available()) {
    state = "";
    while (SerialBT.available()) {
      char incomingChar = SerialBT.read();
      state += String(incomingChar);
    }

    if (state.length() < 9) {
      Serial.print("received: "); Serial.println(state);
      label = state.charAt(0);
      Serial.print("label: "); Serial.println(label);
      switch (label) {
        case 'C':            // Cutoff Voltage
          state.remove(0, 1);
          Serial.print("Tension: "); Serial.println(state);
          tension_limit = state.toDouble();
          start_time = now / 60000;
          AES = '1';
          digitalWrite(AES_Port, HIGH);
          Serial.println("Start tension.");
          break;
        case 'M':                        // Minutes left
          state.remove(0, 1);
          Serial.print("Minutes: "); Serial.println(state);
          minutes_runtime = state.toDouble();
          start_time = now / 60000;
          AES = '1';
          digitalWrite(AES_Port, HIGH);
          Serial.println("Start time.");
          break;
        case 'A':                        // AES state
          state.remove(0, 1);
          Serial.print("AES: "); Serial.println(state);
          AES = state;
          break;
        default:
          // statements
          break;
      }
      state = "";
      Serial.print("received: Tension-limit: "); Serial.print(tension_limit); Serial.print(", minutes_runtime: "); Serial.print(minutes_runtime); Serial.print(", AES: "); Serial.println(AES); Serial.println();
    }
  }
}
void loop() {
  buttonState = digitalRead(pushButton_Port);
  if (buttonState == HIGH) {
    AES = '1';
    Serial.println("AES: on");
    digitalWrite(AES_Port, HIGH);
    Serial.println("Start AES");
    set_LED_colour ('r');
    delay(500);
  }
  now = millis();                       // Store current time
  set_LED_colour('g');
  receive_BT();
  send_BT();
}
