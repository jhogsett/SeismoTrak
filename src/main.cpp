#include <Arduino.h>
#include <Wire.h>
#include <windowed_mean.h>
#include "Adafruit_VL53L0X.h"
#include "zscore.h"

// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31

// set the pins to shutdown
#define SHT_LOX1 6
#define SHT_LOX2 7

// objects for the vl53l0x
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();

// this holds the measurement
VL53L0X_RangingMeasurementData_t measureX;
VL53L0X_RangingMeasurementData_t measureY;

constexpr uint8_t SLEEP_PIN = A0;
constexpr uint32_t TIME_BUDGET = 100000;
constexpr uint32_t CAL_TIME_BUDGET = 200000;

constexpr uint16_t EVENT_WINDOW_SIZE = 10;
constexpr uint16_t BASELINE_WINDOW_SIZE = 300;
constexpr uint16_t PRIMED_VALUE = 45;
constexpr float NOISE_FLOOR = 1.0;
constexpr float EVENT_THRESHOLD = 1.5;
constexpr uint32_t ALARM_RESET_MS = 30000;

constexpr uint8_t EVENT_LED_PIN = 2;
constexpr uint8_t ALARM_LED_PIN = 3;
constexpr uint8_t ACTIVITY_LED_PIN = 2;
constexpr uint8_t ALARM_RESET_PIN = 4;
constexpr uint8_t ALARM_SIREN_PIN = 5;

// constexpr uint8_t CHAMBER_WIDTH = 110;
// constexpr uint8_t WEIGHT_DIAMETER = 34;
// constexpr uint8_t CALIBRATION_SAMPLES = 100;
// constexpr uint8_t MAX_DISPLACEMENT = (CHAMBER_WIDTH - WEIGHT_DIAMETER) / 2;
// uint8_t x_calibrated_center = CHAMBER_WIDTH / 2;
// uint8_t y_calibrated_center = CHAMBER_WIDTH / 2;

constexpr uint8_t ACTIVITY_LED_FILTER = 10;
uint8_t activity_led_iter = 0;

uint32_t event_start_time = 0;
bool alarm_active = false;
bool alarm_suppressed = false;

ZScore zscore_x(EVENT_WINDOW_SIZE, BASELINE_WINDOW_SIZE, PRIMED_VALUE, NOISE_FLOOR, EVENT_THRESHOLD);
ZScore zscore_y(EVENT_WINDOW_SIZE, BASELINE_WINDOW_SIZE, PRIMED_VALUE, NOISE_FLOOR, EVENT_THRESHOLD);

// Function prototypes
void setID();
void event_led_on(bool on = true);
void alarm_led_on(bool on = true);
void alarm_siren_on(bool on = true);
void alarm_reset();
void read_dual_sensors();
void sleep();

/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */
void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activating LOX1 and resetting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2
  if(!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("Failed to boot second VL53L0X"));
    while(1);
  }
}

void event_led_on(bool on){
  digitalWrite(EVENT_LED_PIN, on ? LOW : HIGH);
}

void alarm_led_on(bool on){
  digitalWrite(ALARM_LED_PIN, on ? HIGH : LOW);
}

void alarm_siren_on(bool on){
  digitalWrite(ALARM_SIREN_PIN, on ? HIGH : LOW);
}

void alarm_reset(){
  if(alarm_active && digitalRead(ALARM_RESET_PIN) == LOW){
    Serial.println("Alarm Suppressed");
    // alarm_active = false;
    alarm_suppressed = true;
    // alarm_led_on(false);
    alarm_siren_on(false);
  }
}

void read_dual_sensors() {

  lox1.rangingTest(&measureY, false); // pass in 'true' to get debug data printout!

  activity_led_iter = ++activity_led_iter % ACTIVITY_LED_FILTER; 
  if(activity_led_iter == 0){
    // digitalWrite(ACTIVITY_LED_PIN, LOW);
    event_led_on(true);
  }

  lox2.rangingTest(&measureX, false); // pass in 'true' to get debug data printout!

  // digitalWrite(ACTIVITY_LED_PIN, HIGH);
    event_led_on(false);

  if(measureX.RangeStatus == 4 || measureY.RangeStatus == 4){
    // one or both values is out of range; skip this sample round
    return;
  }

  uint16_t current_x = 0;
  uint16_t current_y = 0;

  current_x = measureX.RangeMilliMeter;
  zscore_x.sample(current_x);
  
  current_y = measureY.RangeMilliMeter;
  zscore_y.sample(current_y);

  // int16_t x_displacement = current_x - x_calibrated_center;
  // int16_t y_displacement = current_y - y_calibrated_center;
  // // int16_t x_displacement = zscore_x.mean() - x_calibrated_center;
  // // int16_t y_displacement = zscore_y.mean() - y_calibrated_center;

  // if (x_displacement > MAX_DISPLACEMENT)  x_displacement = MAX_DISPLACEMENT;
  // if (x_displacement < -MAX_DISPLACEMENT) x_displacement = -MAX_DISPLACEMENT;
  // if (y_displacement > MAX_DISPLACEMENT)  y_displacement = MAX_DISPLACEMENT;
  // if (y_displacement < -MAX_DISPLACEMENT) y_displacement = -MAX_DISPLACEMENT;

  // float x_led_pos = (x_displacement + MAX_DISPLACEMENT) / (MAX_DISPLACEMENT * 2);
  // float y_led_pos = (y_displacement + MAX_DISPLACEMENT) / (MAX_DISPLACEMENT * 2);

  // Serial.print("min:40.0 max:60.0 ");
  // Serial.print("X:");
  // Serial.print(zscore_x.mean());  
  // Serial.print(" Y:");
  // Serial.print(zscore_y.mean());  
  // Serial.print(" XL:");
  // Serial.print(x_displacement);
  // Serial.print(" YL:");
  // Serial.print(y_displacement);

  // digitalWrite(ACTIVITY_LED_PIN, LOW);

  // Serial.print(" XB:");
  // Serial.print(zscore_x.baseline_score());  
  // Serial.print(" YB:");
  // Serial.print(zscore_y.baseline_score());  
  // Serial.print(" XS:");
  // Serial.print(zscore_x.sample_score());  
  // Serial.print(" YS:");
  // Serial.print(zscore_y.sample_score());  
  // Serial.print(" ET:");
  // Serial.print(zscore_x.get_event_triggered() ? 100 : 0);  
  // Serial.print(" EA:");
  // Serial.print(zscore_x.is_event_active() ? 100 : 0);  
  // Serial.println();

  // digitalWrite(ACTIVITY_LED_PIN, HIGH);

  bool event_active = zscore_x.is_event_active() || zscore_y.is_event_active();
  event_led_on(event_active);

  if(event_active){
    event_start_time = millis();
    if(!alarm_suppressed){
      if(!alarm_active){
        Serial.println("Alarm Activated");
      }
      alarm_active = true;
      alarm_led_on(true);
      alarm_siren_on(true);
    }
  } else if(alarm_active){
    if(millis() - event_start_time > ALARM_RESET_MS){

      Serial.println("Alarm Deactivated");
      alarm_active = false;
      alarm_suppressed = false;
      alarm_led_on(false);
      alarm_siren_on(false);

    }
  }

}

// This needed because sometimes the Arduino Nano Every won't accept 
// programming while the event loop is running
void sleep(){
  if(digitalRead(SLEEP_PIN) == LOW){
    Serial.println("Enter sleep mode");
    while(true);
  }
}

// void calibrate() {
//   uint16_t x_sum = 0;
//   uint16_t y_sum = 0;

//   for (int i = 0; i < CALIBRATION_SAMPLES; ) {
//     event_led_on(true);
//     lox1.rangingTest(&measureY, false); // pass in 'true' to get debug data printout!
//     event_led_on(false);
//     alarm_led_on(true);
//     lox2.rangingTest(&measureX, false); // pass in 'true' to get debug data printout!
//     alarm_led_on(false);

//     if(measureX.RangeStatus != 4 || measureY.RangeStatus != 4){
//       x_sum += measureX.RangeMilliMeter;
//       y_sum += measureY.RangeMilliMeter;
//       i++;
//     }
//   }

//   x_calibrated_center = x_sum / CALIBRATION_SAMPLES;
//   y_calibrated_center = y_sum / CALIBRATION_SAMPLES;

//   Serial.print("Calibrated Center Point ");
//   Serial.print("X:");
//   Serial.print(x_calibrated_center);
//   Serial.print(" Y:");
//   Serial.println(y_calibrated_center);

//   delay(2000);
// }


void setup() {
  Serial.begin(115200);

  while (! Serial) { delay(1); }

  Wire.begin();
  Wire.setClock(400000);

  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);

  setID();

  pinMode(SLEEP_PIN, INPUT_PULLUP);

  pinMode(EVENT_LED_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  digitalWrite(EVENT_LED_PIN, HIGH);
  digitalWrite(ALARM_LED_PIN, LOW);

  pinMode(ALARM_RESET_PIN, INPUT_PULLUP);
  pinMode(ALARM_SIREN_PIN, OUTPUT);
  digitalWrite(ALARM_SIREN_PIN, LOW);

  // digitalWrite(EVENT_LED_PIN, LOW);
  // digitalWrite(ALARM_LED_PIN, HIGH);
  // digitalWrite(ALARM_SIREN_PIN, HIGH);
  event_led_on(true);
  alarm_led_on(true);
  alarm_siren_on(true);
  delay(500);
  // digitalWrite(EVENT_LED_PIN, HIGH);
  // digitalWrite(ALARM_LED_PIN, LOW);
  // digitalWrite(ALARM_SIREN_PIN, LOW);
  event_led_on(false);
  alarm_led_on(false);
  alarm_siren_on(false);

  // lox1.setMeasurementTimingBudgetMicroSeconds(CAL_TIME_BUDGET); // 20 ms timing budget for high speed
  // lox2.setMeasurementTimingBudgetMicroSeconds(CAL_TIME_BUDGET); // 20 ms timing budget for high speed

  // calibrate();

  lox1.setMeasurementTimingBudgetMicroSeconds(TIME_BUDGET); // 20 ms timing budget for high speed
  lox2.setMeasurementTimingBudgetMicroSeconds(TIME_BUDGET); // 20 ms timing budget for high speed

  Serial.println("Ready");
}

void loop() {
  sleep();  
  alarm_reset();
  read_dual_sensors();
}


// TODO: 
// add neopixel library, pin, and dynamic active alarm sound
// add mosfet switch for alarm siren
// perf board
