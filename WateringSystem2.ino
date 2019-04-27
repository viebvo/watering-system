
//----------------------------------------------------------------------------------------------
//        PlantWateringSystem2 Rev.2
//----------------------------------------------------------------------------------------------

// LIBRARIES
//----------------------------------------------------------------------------------------------
// none

// USER DEFINED CONSTANTS
//----------------------------------------------------------------------------------------------
// define the number of seconds after which watering system should check. Note that this is calculated as unsigned long to ensure possibility of longer time periods
const unsigned long checkintervalshort = 4UL * 60UL * 60UL; // (4 hours * 60 minutes * 60 seconds)
const unsigned long checkintervallong = 5UL * 24UL * 60UL * 60UL; // (5 days * 24 hours * 60 minutes * 60 seconds)

const int TestSwitch = 2; // the pin number for the check switch. cabling: pin->switch->ground
const int ModeSwitch = 3; // the pin number for the Mode switch. Default=high (pullup), so pump motor will run until sensor is wet. cabling: pin->switch->ground

const int W_channels = 4; // number of sensor/pump combinations. expand below arrays to add other channel(s)
const int W_VCC[] = {4, 7, A0, A3}; // VCC of YL-38 comparator
const int W_read[] = {5, 8, A1, A4}; // digital out of YL-38 comparator
const int W_pump[] = {6, 9, A2, A5}; // water pump enable pin
// define here how long the pump(s) should run. No runtime but sensor precedence if ModePIN is enabled!
const int W_runtime[] = {60, 60, 60, 60};

const int R_VCC = 10; // VCC of water reservoir
const int R_read = 11; // digital out for reservoir YL-38 comparator

const int LED_red = 13;

// RUNTIME VARIABLES
//----------------------------------------------------------------------------------------------
unsigned long actTime;
unsigned long timeelapsed = 0;
unsigned long currentinterval = 0;  // first interval=zero for immediate start after power-on
unsigned long checkWaterSecs = 0;   // will store last time the water was checked
unsigned long checkRuntimeSecs = 0;   // will store last time the motor was checked
//----------------------------------------------------------------------------------------------

void setup() {

  Serial.begin(9600);
  Serial.println("PlantWateringSystem2 Rev.2");
  Serial.print(W_channels); Serial.println(" pump(s) enabled");
  Serial.print("Check interval short: "); Serial.print(checkintervalshort / 60 / 60); Serial.println(" hour(s)");
  Serial.print("Check interval long: "); Serial.print(checkintervallong / 60 / 60); Serial.println(" hour(s)");

  // set the switch pin as input with a pullup resistor to ensure it defaults to HIGH
  pinMode(TestSwitch, INPUT_PULLUP);
  pinMode(ModeSwitch, INPUT_PULLUP);

  pinMode (LED_red, OUTPUT);
  digitalWrite (LED_red, LOW);
  pinMode (R_VCC, OUTPUT);
  pinMode(R_read, INPUT_PULLUP);

  // initialize all i/o
  for (int i = 0; i <= W_channels - 1; i++) {
    pinMode(W_VCC[i], OUTPUT);
    pinMode(W_read[i], INPUT_PULLUP);
    pinMode(W_pump[i], OUTPUT);
    digitalWrite (W_pump[i], LOW);
    Serial.print("Pump run time channel "); Serial.print(i + 1); Serial.print(": "); Serial.print(W_runtime[i]); Serial.println(" sec");
  }
}


//----------------------------------------------------------------------------------------------

void loop() {
  delay(100);
  readTestSwitch();
  actTime = millis() / 1000UL;
  if ((unsigned long)(actTime - checkWaterSecs) >= currentinterval) {
    checkWaterSecs = actTime;
    if (readReservoir()) { // TRUE -> enough water in the reservoir
      digitalWrite (LED_red, LOW); // delete red flag
      for (int i = 0; i <= W_channels - 1; i++) {
        Serial.println("");
        if (!readPot(W_VCC[i], W_read[i])) { 
          Serial.print("Channel "); Serial.print(i + 1); Serial.println(": TOO DRY");
          if (digitalRead(ModeSwitch) == HIGH) { // ModeSwitch is HIGH...run until full!
            Serial.println("ModePIN is set to sensor feedback");
            MotorON (W_pump[i]);
            checkRuntimeSecs = 1;
            delay (1000);
            while ((!readPot(W_VCC[i], W_read[i])) && readReservoir()) {
              // wait until sensor tells full condition
              Serial.print(".");
              delay (1000);
              checkRuntimeSecs++;
              if (checkRuntimeSecs > W_runtime[i]) {
                // if we have exceeded the maximum runtime
                MotorOFF (W_pump[i]);
                Serial.println("Watering took too long! Please check and reset!");
                while (1) {
                  // let the error LED blink until user intervention
                  digitalWrite (LED_red, HIGH);
                  delay (100);
                  digitalWrite (LED_red, LOW);
                  delay (100);
                }
              }
            }
          }
          else {
            checkRuntimeSecs = 1;
            MotorON (W_pump[i]);
            while ((checkRuntimeSecs <= W_runtime[i]) && readReservoir()) {
              //Serial.println(checkRuntimeSecs);
              delay (1000);
              checkRuntimeSecs++;
            }
          }
          MotorOFF (W_pump[i]);

        } else {
          Serial.print("Channel "); Serial.print(i + 1); Serial.println(": WET ENOUGH");
        }
      }
    }
    if (!readReservoir()) {
      digitalWrite (LED_red, HIGH); // reservoir is empty, set red flag
      //digitalWrite (R_VCC, LOW); // disable reservoir YL-38
      Serial.println("No Water in reservoir!");
      readModeSwitch();
      // because of empty reservoir we lower the interval and wait a minute to re-check
      checkWaterSecs = actTime + currentinterval;
      delay (60000);
    }
  }
  readModeSwitch();
}

//----------------------------------------------------------------------------------------------

void MotorON (int pin) {
  digitalWrite (pin, HIGH);
  //Serial.print ("PIN"); Serial.print(pin); Serial.println(" PUMP ON");
}
//----------------------------------------------------------------------------------------------
void MotorOFF (int pin) {
  digitalWrite (pin, LOW);
  Serial.print ("PIN"); Serial.print(pin); Serial.println(" PUMP OFF");
}
//----------------------------------------------------------------------------------------------

bool readReservoir() {
  
  digitalWrite (R_VCC, HIGH);
  delay (100);
  if (digitalRead(R_read) == LOW) {
    //Serial.println("Checking Reservoir...full");
    digitalWrite (R_VCC, LOW);
    return true;
  }
  else
  {
    //Serial.println("Checking Reservoir...empty");
    digitalWrite (R_VCC, LOW);
    return false;
  }
}
//----------------------------------------------------------------------------------------------
bool readPot(int pin_vcc, int pin_val) {
  
  digitalWrite (pin_vcc, HIGH);
  delay (100);
  if (digitalRead(pin_val) == LOW) {
    //Serial.println("Checking Pot...full");
    digitalWrite (pin_vcc, LOW);
    return true;
  }
  else
  {
    //Serial.println("Checking Pot...empty");
    digitalWrite (pin_vcc, LOW);
    return false;
  }
}
//----------------------------------------------------------------------------------------------

void readTestSwitch() {
  if (digitalRead(TestSwitch) == LOW) {
    //Serial.println("TestPIN is engaged");
    for (int i = 0; i <= W_channels - 1; i++) {
      digitalWrite (W_VCC[i], HIGH);
    }
    digitalWrite (R_VCC, HIGH);
  }
  else {
    //Serial.println("TestPIN is idle");
    for (int i = 0; i <= W_channels - 1; i++) {
      digitalWrite (W_VCC[i], LOW);
    }
    digitalWrite (R_VCC, LOW);
  }
}
//----------------------------------------------------------------------------------------------

void readModeSwitch() {
  if (digitalRead(ModeSwitch) == HIGH) { // Switch idle or no switch connected - default to short interval
    currentinterval = checkintervalshort;
  }
  else {
    currentinterval = checkintervallong;
  }
}
//----------------------------------------------------------------------------------------------
