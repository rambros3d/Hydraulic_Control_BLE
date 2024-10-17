
#include <Arduino.h>
#include <ESP32Servo.h>  //  https://github.com/madhephaestus/ESP32Servo
#include <AceRoutine.h>  //  https://github.com/bxparks/AceRoutine
using namespace ace_routine;

// RemoteXY select connection mode and include library
#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
#include <RemoteXY.h>

// RemoteXY connection settings
#define REMOTEXY_BLUETOOTH_NAME "Rambros_hydraulics"
#define REMOTEXY_ACCESS_PASSWORD "1234"

// you can enable debug logging to Serial at 115200
//#define REMOTEXY__DEBUGLOG

//////////////////////////////////////////////
//     RemoteXY include configuration       //
//////////////////////////////////////////////

uint8_t RemoteXY_CONF[] =  // 151 bytes
  { 255, 2, 0, 2, 0, 144, 0, 19, 0, 0, 0, 0, 25, 1, 106, 200, 1, 1, 5, 0,
    4, 3, 116, 101, 16, 176, 177, 26, 71, 12, 51, 47, 47, 56, 19, 94, 31, 165, 0, 0,
    0, 0, 0, 0, 200, 66, 0, 0, 160, 65, 0, 0, 32, 65, 0, 0, 160, 64, 31, 112,
    111, 119, 101, 114, 0, 135, 0, 0, 0, 0, 0, 0, 72, 66, 94, 0, 0, 72, 66, 0,
    0, 160, 66, 36, 0, 0, 160, 66, 0, 0, 200, 66, 10, 69, 64, 22, 20, 113, 177, 26,
    31, 80, 85, 77, 80, 0, 31, 129, 15, 27, 77, 8, 0, 19, 51, 68, 32, 80, 114, 105,
    110, 116, 101, 100, 32, 72, 121, 100, 114, 97, 117, 108, 105, 99, 115, 0, 129, 31, 15, 45,
    11, 64, 31, 82, 97, 109, 66, 114, 111, 115, 0 };

// this structure defines all the variables and events of your control interface
struct {

  // input variables
  int8_t slider;       // from -100 to 100
  uint8_t pumpEnable;  // =1 if state is ON, else =0

  // output variables
  int16_t instrument;  // from 0 to 100

  // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;


/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

Servo PumpEsc;     // create servo object to control a Pump
Servo ValveServo;  // create servo object to control a valve

#define PUMP_MIN_US 1500
#define PUMP_MAX_US 2000

#define MID_ANGLE 90
#define MOVE_ANGLE 80

int16_t valveAngle = 0;
int16_t pumpUs = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  RemoteXY_Init();
  RemoteXY.instrument = 0;

  ValveServo.setPeriodHertz(50);
  ValveServo.attach(S2, 500, 2400);
  ValveServo.write(MID_ANGLE);

  PumpEsc.setPeriodHertz(50);
  PumpEsc.attach(S1, PUMP_MIN_US, PUMP_MAX_US);
  PumpEsc.writeMicroseconds(PUMP_MIN_US);
  delay(200);

  digitalWrite(LED_BUILTIN, LOW);

  CoroutineScheduler::setup();
}


void loop() {
  RemoteXY_Handler();
  digitalWrite(LED_BUILTIN, (RemoteXY.pumpEnable == 0) ? LOW : HIGH);

  CoroutineScheduler::loop();
}

COROUTINE(ValveControl) {
  COROUTINE_LOOP() {
    valveAngle = map(RemoteXY.slider, -100, 100, MID_ANGLE + MOVE_ANGLE, MID_ANGLE - MOVE_ANGLE);
    ValveServo.write(valveAngle);

    COROUTINE_DELAY(20);
  }
}

COROUTINE(PumpControl) {
  COROUTINE_LOOP() {

    if (RemoteXY.pumpEnable == 1) {
      int8_t sliderVal = abs(RemoteXY.slider) - 5;
      sliderVal = constrain(sliderVal, 0, 100);
      pumpUs = map(sliderVal, 0, 100, PUMP_MIN_US, PUMP_MAX_US);
      PumpEsc.writeMicroseconds(pumpUs);
    } else {
      PumpEsc.writeMicroseconds(PUMP_MIN_US);
    }
    RemoteXY.instrument = map(pumpUs, PUMP_MIN_US, PUMP_MAX_US, 0, 100);

    COROUTINE_DELAY(20);
  }
}

COROUTINE(health) {
  COROUTINE_LOOP() {
    if (RemoteXY.connect_flag == 0) {
      RemoteXY.slider = 0;
      RemoteXY.pumpEnable = 0;
    }
    COROUTINE_DELAY(200);  //  Check health 5 times/second
  }
}