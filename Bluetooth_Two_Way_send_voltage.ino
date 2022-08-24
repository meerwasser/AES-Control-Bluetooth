/*______Import Libraries_______*/
// this header is needed for Bluetooth Serial -> works ONLY on ESP32
#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "BLEDevice.h"
/*______End of Libraries_______*/


#define HM_MAC "60:A4:23:91:94:42"  // Adresse über nRF Connect App finden !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!B O N D I N G nicht vergessen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
uint32_t PIN = 173928 ;             // Bonding Passwort

// Service und Characteristic des Votronic Bluetooth Connectors
static BLEUUID Votronic_serviceUUID("d0cb6aa7-8548-46d0-99f8-2d02611e5270");
static BLEUUID Batteriecomputer_charUUID("9a082a4e-5bcc-4b1d-9958-a97cfccfa5ec");
static BLEUUID Solarregler_charUUID("971ccec2-521d-42fd-b570-cf46fe5ceb65");

//Variablen, die immer über die Callbacks aktualisiert werden
//Wert bleibt im Sleep-Modus erhalten
RTC_DATA_ATTR float Spannung_Aufbaubatterie, Strom_BC, Strom_SR, Spannung_Starterbatterie;
RTC_DATA_ATTR int16_t Batterieladungsstatus, Ladung_SR_Wh, Ladung_SR_Ah;
RTC_DATA_ATTR String Solarstatus;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic_BC;
static BLERemoteCharacteristic* pRemoteCharacteristic_SR;
BLEClient*  pClient;

class MySecurity : public BLESecurityCallbacks {
    bool onConfirmPIN(uint32_t pin) {
      return false;
    }
    uint32_t onPassKeyRequest() {
      ESP_LOGI(LOG_TAG, "PassKeyRequest");
      //delay(1000);
      return PIN;
    }
    void onPassKeyNotify(uint32_t pass_key) {
      ESP_LOGI(LOG_TAG, "On passkey Notify number:%d", pass_key);
    }
    bool onSecurityRequest() {
      ESP_LOGI(LOG_TAG, "On Security Request");
      return true;
    }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
      ESP_LOGI(LOG_TAG, "Starting BLE work!");
      if (cmpl.success) {
        uint16_t length;
        esp_ble_gap_get_whitelist_size(&length);
        ESP_LOGD(LOG_TAG, "size: %d", length);
      }
    }
};



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
double minutes_runtime = 240;                    //actual minutes set
double time_left = 240;                     //actual runtime until cut-off
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

  Serial.println("Read Votronic BLE Connector");
  Setup_Votronic();

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

// Batteriecomputer Callback
static void Batteriecomputer_Callback (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  int16_t iStrom;
  Spannung_Aufbaubatterie = float(pData[1] << 8 | pData[0]) / 100;
  Spannung_Starterbatterie = float(pData[3] << 8 | pData[2]) / 100;
  iStrom = pData[11] << 8 | pData[10];
  Strom_BC = float(iStrom) / 1000;
  Batterieladungsstatus = pData[8];
  Serial.print("Batterieladungsstatus: ");
  Serial.print(Batterieladungsstatus);
  Serial.print("          Stromfluss: ");
  Serial.print(Strom_BC);
  Serial.print("      Spannung_Aufbaubatterie: ");
  Serial.println(Spannung_Aufbaubatterie);
}

// Solarregler Callback
static void Solarregler_Callback (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Strom_SR = float(pData[5] << 8 | pData[4]) / 10; ;
  if (pData[12] == 9) {
    Solarstatus = "Aktiv";
  }
  else if (pData[12] == 25) {
    Solarstatus = "Stromreduzierung";
  }
  else {
    Solarstatus = String(pData[12]);
  }
  Ladung_SR_Wh = (pData[16] << 8 | pData[15]) * 10;
  Ladung_SR_Ah = (pData[14] << 8 | pData[13]);
  Serial.print("Solarstrom: ");
  Serial.print(Strom_SR);
  Serial.print("  |  ");
  Serial.print("Solarstatus: ");
  Serial.print(Solarstatus);
  Serial.print("  |  ");
  Serial.print("Ladung Wh: ");
  Serial.print(Ladung_SR_Wh);
  Serial.print("  |  ");
  Serial.print("Ladung Ah: ");
  Serial.println(Ladung_SR_Ah);
}

// Verbinde mit Votronic BLE Server.
bool connectToServer(BLEAddress pAddress)
{
  Serial.print("Verbinde mit ");
  Serial.println(pAddress.toString().c_str());

  //BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT );
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT );
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND); //
  pSecurity->setCapability(ESP_IO_CAP_OUT);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pClient = BLEDevice::createClient();

  if ( pClient->connect(pAddress) ) {
    Serial.println("BLE Verbunden");
    BLERemoteService* pRemoteService = pClient->getService(Votronic_serviceUUID);
    if (pRemoteService == nullptr)
    {
      Serial.print("Service UUID nicht gefunden: ");
      Serial.println(Votronic_serviceUUID.toString().c_str());
      return false;
    }
    //Callback für Batteriecomputer
    pRemoteCharacteristic_BC = pRemoteService->getCharacteristic(Batteriecomputer_charUUID);
    if (pRemoteCharacteristic_BC == nullptr) {
      Serial.print("Characteristic UUID nicht gefunden: ");
      Serial.println(Batteriecomputer_charUUID.toString().c_str());
      return false;
    }
    pRemoteCharacteristic_BC->registerForNotify(Batteriecomputer_Callback);
    Serial.println("Callback Batteriecomputer_Callback");
    Batteriecomputer_Callback;
    Serial.print("Batterieladungsstatus: ");  Serial.println(Batterieladungsstatus);
    //Callback für Solarregler
    pRemoteCharacteristic_SR = pRemoteService->getCharacteristic(Solarregler_charUUID);
    if (pRemoteCharacteristic_SR == nullptr) {
      Serial.print("Characteristic UUID nicht gefunden: ");
      Serial.println(Batteriecomputer_charUUID.toString().c_str());
      return false;
    }

    pRemoteCharacteristic_SR->registerForNotify(Solarregler_Callback);
    Serial.println("Callback Solarregler_Callback");
  }
  else {
    Serial.println("BLE nicht Verbunden");
  }
  return true;
}

//BLE Connection und Listener auf Werte des Batteriecomputers und Solarreglers
void Setup_Votronic() {
  bool connect_to = false;
  while (connect_to == false) {
    BLEDevice::init("");
    pServerAddress = new BLEAddress(HM_MAC);
    connectToServer(*pServerAddress);
  }
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
    adc0 = adc0 * 0.000715;
    adc0 = adc0 + 0.07388;
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
      digitalWrite(Kill_Port, LOW);
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
    voltage = 18 -  tension_lost;
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
  Serial.print("buttonState: "); Serial.println(buttonState);
  if (buttonState == HIGH) {
    standalone_op();
  }
  now = millis();                       // Store current time

  receive_BT();
  send_BT();
}