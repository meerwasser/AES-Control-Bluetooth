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
  last_sent = millis();
  Serial.begin(115200);
  SerialBT.begin("ESP32_Control");        // Name of your Bluetooth interface -> will show up on your phone

}

void simulate() {
  double factor = random(0, 60); // create random number
  voltage = 15 -  (factor / 10);
  actual_minutes = now / 60000;
  time_left = minutes_runtime - actual_minutes;

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

    delay(500);
    Serial.print("Volt: "); Serial.println(sent_char);
    last_sent = millis();
  }
}

void receive_BT() {

  if (SerialBT.available()) {
    state = "";
      while (SerialBT.available()) {
        char incomingChar = SerialBT.read();
        state += String(incomingChar);
      }

     if (state.length() < 9) {

        Serial.print("received: "); Serial.println(state);
        actual_minutes = state.toInt();
        state = "";
    }
  }
}
  void loop() {
    now = millis();                       // Store current time
    receive_BT();
    send_BT();
  }
