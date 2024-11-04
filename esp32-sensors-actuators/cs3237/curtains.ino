#include <ESP32Servo.h>

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
// Possible PWM GPIO pins on the ESP32-S2: 0(used by on-board button),1-17,18(used by on-board LED),19-21,26,33-42
// Possible PWM GPIO pins on the ESP32-S3: 0(used by on-board button),1-21,35-45,47,48(used by on-board LED)
// Possible PWM GPIO pins on the ESP32-C3: 0(used by on-board button),1-7,8(used by on-board LED),9-10,18-21
// #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
// int servoPin = 17;
// #elif defined(CONFIG_IDF_TARGET_ESP32C3)
// int servoPin = 7;
// #else
// int servoPin = 18;
// #endif

int servoPin = 2;

int bottomDetectorPin = 13;
int topDetectorPin = 12;
int raiseButton = 14;
int lowerButton = 27;

bool lowerCurtain = false;
bool raiseCurtain = false;
bool isLoweringCurtain = false;
bool isRaisingCurtain = false;

void startLowerCurtain() {
  if (!isLoweringCurtain) {
    myservo.write(0);
    Serial.println("Lowering!");
    isLoweringCurtain = true;
  }
}

void stopCurtain() {
  isLoweringCurtain = false;
  isRaisingCurtain = false;
  myservo.write(90);
  Serial.println("Stopping!");
}

void startRaiseCurtain() {
  if (!isRaisingCurtain) {
    myservo.write(180);
    Serial.println("Raising!");
    isRaisingCurtain = true;
  }
}


void setup() {
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object
	// using default min/max of 1000us and 2000us
	// different servos may require different min/max settings
	// for an accurate 0 to 180 sweep

  myservo.write(90);
  Serial.begin(115200);
  // buttons
  pinMode(raiseButton, INPUT);
  pinMode(lowerButton, INPUT_PULLUP);

  // distance detection sensors
  pinMode(bottomDetectorPin, INPUT);
  pinMode(topDetectorPin, INPUT);
}

void loop() {
  if (!digitalRead(raiseButton)) { // trigger once to raise
    delay(50);
    Serial.println("Raise button pressed!");
    raiseCurtain = 1;
    lowerCurtain = 0;
  }

  if (!digitalRead(lowerButton)) { // trigger once to lower
    delay(50);
    Serial.println("Lower button pressed!");
    lowerCurtain = 1;
    raiseCurtain = 0;
  }

  if (lowerCurtain && digitalRead(bottomDetectorPin)) {
    startLowerCurtain();
  } else if (lowerCurtain && !digitalRead(bottomDetectorPin)) {
    lowerCurtain = 0;
    stopCurtain();
  }

  if (raiseCurtain && !digitalRead(topDetectorPin)) {
    startRaiseCurtain();
  } else if (raiseCurtain && digitalRead(topDetectorPin)) {
    raiseCurtain = 0;
    stopCurtain();
  }
}


