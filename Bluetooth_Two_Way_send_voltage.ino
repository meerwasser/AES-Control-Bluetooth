/*______Import Libraries_______*/
// this header is needed for Bluetooth Serial -> works ONLY on ESP32
#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
/*______End of Libraries_______*/

#define sent_intervall 1000 //2 sec time between sending

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
int simulate = 0;         // if simulate = 1, sketch runs without other hardware
int AES_triggered = 0; //switch was trigerred

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

  last_sent = millis();
  Serial.begin(115200);
  if (simulate == 0) {
    start_analog_digital_converter();
  }
  SerialBT.begin("AES_Contr_ESP");        // Name of your Bluetooth interface -> will show up on your phone
  pinMode(green_LED, OUTPUT);    // sets the LED pins as output
  pinMode(red_LED, OUTPUT);    // sets the LED pins as output
  pinMode(blue_LED, OUTPUT);    // sets the LED pins as outputgreen_LED
  pinMode(AES_Port, OUTPUT);
  pinMode(Kill_Port, OUTPUT);

  pinMode(pushButton_Port, INPUT);
  power_control('r'); //r: ready, a: AES, o: off

}

void start_analog_digital_converter() {
  Serial.println(F("ADS1115 test"));
  Serial.println("Getting Single-Ended reading from AIN0 (P)");

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
  set_LED_colour('g');
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
  if (simulate == 1) {
    // simulate Voltage
    double factor = random(0, 100); // create random number
    tension_lost = tension_lost + (factor / 1000);
    voltage = 15 -  tension_lost;
    // end simulate
  } else {
    read_voltage();
  }
  if (voltage <= tension_limit) {
    AES = '0';
    Serial.print("Tension low. Tension_limit: "); Serial.print(tension_limit); Serial.print(" Voltage: "); Serial.println(voltage);
    Serial.println(" ");
    if (AES_triggered == 0) {
      power_control('o'); //r: ready, a: AES, o: off
    }
    set_LED_colour ('g');
    AES_triggered = 1;
  }


  /* ##############################
      time
    ###############################*/
  time_to_go();
  if (time_left <= 0) {
    AES = '0';
    Serial.print("Time out. Time_left: "); Serial.println(time_left);
    Serial.println(" ");
    if (AES_triggered == 0) {
      power_control('o'); //r: ready, a: AES, o: off
    }
    set_LED_colour ('g');
    AES_triggered = 1;
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

/*###########################################
   Power-status
   r: ready (Power on, AES off)
   a: AES    (Power on, AES on
   o: off    (Power off, AES off)
  ############################################*/

void power_control(char power) {
  switch (power) {
    case 'r':
      Serial.println(" ");
      Serial.println("Power ready: Power on, AES off");
      digitalWrite(AES_Port, LOW);
      digitalWrite(Kill_Port, LOW);
      break;
    case 'a':
      set_LED_colour ('r');
      Serial.println(" ");
      Serial.println("Power AES: Power on, AES on");
      digitalWrite(AES_Port, HIGH);
      digitalWrite(Kill_Port, LOW);
      break;
    case 'o':
      Serial.println(" ");
      Serial.println("Power off: Power off, AES off");
      digitalWrite(AES_Port, LOW);
      digitalWrite(Kill_Port, HIGH);
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
    Serial.print("Volt: "); Serial.println(sent_char);
    last_sent = millis();

  }
}

void receive_BT() {
  char label = ' ';
  double tens_temp = 11.5;

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
          tens_temp = state.toDouble();
          if (tens_temp >= 11.5) {
            tension_limit = tens_temp;
          }
          start_time = now / 60000;
          AES = '1';
          power_control('a'); //r: ready, a: AES, o: off
          Serial.println("Start tension.");
          set_LED_colour ('b');
          AES_triggered = 0;
          break;
        case 'M':                        // Minutes left
          state.remove(0, 1);
          Serial.print("Minutes: "); Serial.println(state);
          minutes_runtime = state.toDouble();
          start_time = now / 60000;
          AES = '1';
          power_control('a'); //r: ready, a: AES, o: off
          Serial.println("Start time.");
          set_LED_colour ('b');
          AES_triggered = 0;
          break;
        case 'A':                        // AES state
          state.remove(0, 1);
          Serial.print("AES: "); Serial.println(state);
          AES = state;
          AES_triggered = 0;
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

void standalone_op() {
  AES = '1';
  start_time = now / 60000;
  tension_limit = 12.9;
  minutes_runtime = 240;
  power_control('a'); //r: ready, a: AES, o: off
  set_LED_colour ('r');
  AES_triggered = 0;
}

void loop() {
  if (AES_triggered == 1) {
    power_control('o');
  }
  buttonState = digitalRead(pushButton_Port);
  if (buttonState == HIGH) {
    standalone_op();
  }
  now = millis();                       // Store current time

  receive_BT();
  send_BT();
}
