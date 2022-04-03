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
  pinMode(green_LED, OUTPUT);    // sets the LED pins as output
  pinMode(red_LED, OUTPUT);    // sets the LED pins as output
  pinMode(blue_LED, OUTPUT);    // sets the LED pins as outputgreen_LED
  pinMode(Kill_Port, OUTPUT);
  pinMode(AES_Port, OUTPUT);

  pinMode(pushButton_Port, INPUT);

  digitalWrite(blue_LED, HIGH);
  digitalWrite(Kill_Port, HIGH);
  delay(500);
  digitalWrite(Kill_Port, LOW);

  last_sent = millis();
  Serial.begin(115200);
  digitalWrite(green_LED, HIGH);
  delay(500);
  if (simulate == 0) {
    start_analog_digital_converter();
  }
  SerialBT.begin("AES_Contr_ESP");        // Name of your Bluetooth interface -> will show up on your phone
  digitalWrite(red_LED, HIGH);
  delay(500);
  digitalWrite(green_LED, LOW);
  digitalWrite(red_LED, LOW);
  digitalWrite(blue_LED, LOW);
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

/*###########################################
   Power-status
   r: ready (Power on, AES off)
   a: AES    (Power on, AES on
   o: off    (Power off, AES off)
  ############################################*/

void power_control(char power) {
  switch (power) {
    case 'r':
      //   Serial.println(" ");
      // Serial.println("Power ready: Power on, AES off");
      digitalWrite(AES_Port, LOW);
      digitalWrite(Kill_Port, LOW);
      digitalWrite(green_LED, HIGH);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, LOW);
      break;
    case 'a':
      //   Serial.println(" ");
      //  Serial.println("Power AES: Power on, AES on");
      digitalWrite(AES_Port, HIGH);
      digitalWrite(Kill_Port, LOW);
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, HIGH);
      break;
    case 'o':
      //     Serial.println(" ");
      Serial.println("Power off: Power off, AES off");
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, LOW);
      digitalWrite(AES_Port, LOW);
      digitalWrite(Kill_Port, HIGH);
      delay(1000);
      Serial.println("Delay off");
      break;
    case 's':
      //     Serial.println(" ");
      Serial.println("Power off: Power off, AES off");
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, HIGH);
      digitalWrite(blue_LED, LOW);
      digitalWrite(AES_Port, HIGH);
      digitalWrite(Kill_Port, HIGH);
      delay(1000);
      Serial.println("Delay off");
      break;
    default:
      // statements
      break;
  }
}
void check_state() {
  if (AES == "0") {
    power_control('r'); //r: ready, a: AES, o: off
  }
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
    Serial.print("Tension low. Tension_limit: "); Serial.print(tension_limit); Serial.print(" Voltage: "); Serial.println(voltage); Serial.println(" ");
    power_control('o'); //r: ready, a: AES, o: off
  } else {
    if (AES == "1") {
      power_control('a'); //r: ready, a: AES, o: off
    }
    if (AES == "0") {
      power_control('r'); //r: ready, a: AES, o: off
    }
  }
  /* ##############################
      time
    ###############################*/
  time_to_go();
  if (time_left <= 0) {
    AES = '0';
    Serial.print("Time out. Time_left: "); Serial.println(time_left); Serial.println(" ");
    power_control('o'); //r: ready, a: AES, o: off
  } else {
    if (AES == "1") {
      power_control('a'); //r: ready, a: AES, o: off
    }
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
    read_status();
    SerialBT.println(sent_char);
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
    Serial.print("Bluetooth received!"); Serial.print("received: "); Serial.println(state);
    if (state.length() < 9) {
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
          break;
        case 'M':                        // Minutes left
          state.remove(0, 1);
          Serial.print("Minutes: "); Serial.println(state);
          minutes_runtime = state.toDouble();
          start_time = now / 60000;
          AES = '1';
          power_control('a'); //r: ready, a: AES, o: off
          Serial.println("Start time.");
          break;
        case 'A':                        // AES state
          state.remove(0, 1);
          Serial.print("AES: "); Serial.println(state);
          AES = state;
          break;
        default:
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
  power_control('s'); //r: ready, a: AES, o: off
}

void loop() {
  check_state();

  buttonState = digitalRead(pushButton_Port);
  if (buttonState == HIGH) {
    standalone_op();
  }
  now = millis();                       // Store current time

  receive_BT();
  send_BT();
}
