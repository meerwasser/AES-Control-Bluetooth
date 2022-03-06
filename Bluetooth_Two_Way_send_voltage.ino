// --------------------------------------------------
//
// ESP32 Bluetooth App part 2 -> bi-directional cummunication
//
// Code for bi-directional Bluetooth communication demonstration between the ESP32 and mobile phone (with MIT inventor app).
// device used for tests: ESP32-WROOM-32D
//

//
// --------------------------------------------------

// this header is needed for Bluetooth Serial -> works ONLY on ESP32
#include "BluetoothSerial.h"

#define sent_intervall 2000 //2 sec time between sending


#define green_LED  18
#define red_LED  33
#define blue_LED  23


// Parameters for Bluetooth interface and timing
int incoming;                           // variable to store byte received from phone
unsigned long now;                      // variable to store current "time" using millis() function
unsigned long last_sent;                // variable to store point in time of sending


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
  SerialBT.begin("ESP32_Control");        // Name of your Bluetooth interface -> will show up on your phone
  pinMode(green_LED, OUTPUT);    // sets the LED pins as output
  pinMode(red_LED, OUTPUT);    // sets the LED pins as output
  pinMode(blue_LED, OUTPUT);    // sets the LED pins as outputgreen_LED

}

void simulate() {
  double factor = random(0, 60); // create random number
  voltage = 15 -  (factor / 10);
  actual_minutes = now / 60000;
  time_left = minutes_runtime - actual_minutes;
}

void set_LED_colour(char LED_Colour) {
  switch (LED_Colour) {
    case 'g':
      digitalWrite(green_LED, HIGH);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, LOW);
      break;
    case 'r':
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, HIGH);
      digitalWrite(blue_LED, LOW);
      break;
    case 'b':
      digitalWrite(green_LED, LOW);
      digitalWrite(red_LED, LOW);
      digitalWrite(blue_LED, HIGH);
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
    simulate();
    read_status();
    SerialBT.println(sent_char);
    set_LED_colour ('b');
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
          break;
        case 'M':                        // Minutes left
          state.remove(0, 1);
          Serial.print("Minutes: "); Serial.println(state);
          actual_minutes = state.toDouble();
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
      Serial.print("received: Tension-limit: "); Serial.print(tension_limit); Serial.print(", actual_minutes: "); Serial.print(actual_minutes); Serial.print(", AES: "); Serial.println(AES); Serial.println();
    }
  }
}
void loop() {
  now = millis();                       // Store current time
  set_LED_colour('g');
  receive_BT();
  send_BT();
}
