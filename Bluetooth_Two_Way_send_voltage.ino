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

#define sent_intervall 500 //0,5 sec time between sending


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
    state = SerialBT.read();
    Serial.print("received: "); Serial.println(state);
    actual_minutes = state.toInt();
  }
}


void loop() {
  now = millis();                       // Store current time
  receive_BT();
  send_BT();


  /*

    // -------------------- Receive Bluetooth signal ----------------------
    if (SerialBT.available())
    {
      incoming = SerialBT.read(); //Read what we receive and store in "incoming"

      // separate button ID from button value -> button ID is 10, 20, 30, etc, value is 1 or 0
      int button = floor(incoming / 10);
      int value = incoming % 10;

      switch (button) {
        case 1:
          Serial.print("Button 1:"); Serial.println(value);
          digitalWrite(led_pin_1, value);
          if (value == 1)                                           // check if the button is switched on
            time_button1 = now;                                     // if button is switched on, write the current time to the timestamp
          break;
        case 2:
          Serial.print("Button 2:"); Serial.println(value);
          digitalWrite(led_pin_2, value);
          if (value == 1)
            time_button2 = now;
          break;
        case 3:
          Serial.print("Button 3:"); Serial.println(value);
          digitalWrite(led_pin_3, value);
          if (value == 1)
            time_button3 = now;
          break;
      }
    }
  */
}
