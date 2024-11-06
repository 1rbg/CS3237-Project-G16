#include <Arduino.h>
#include <ADS1115_WE.h>
#include <Wire.h>
#include <FastLED.h>
#include <TimerFreeTone.h>
#include <WiFi.h>
#include "ESP32MQTTClient.h"
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
// int lowerButton = 27;

bool lowerCurtain = false;
bool raiseCurtain = false;
bool isLoweringCurtain = false;
bool isRaisingCurtain = false;
int isOpen = 2;

const char *ssid = "Galaxy S21+ 5Gf502";
const char *pass = "fmkz9731";

char *server = "mqtt://192.168.237.245:1883";

char *subscribeTopic = "tray-return/actuate"; // bird detected
char *publishTopic = "tray-return/sensor"; 

unsigned long previousMillis = 0;    // Stores the last time the function was called
const long interval = 5000;


ESP32MQTTClient mqttClient;

#define NUM_LEDS 4          // Number of LEDs on the strip
#define NUM_TRAYS 8         // Number of trays
#define DATA_PIN 33         // LED data pin
#define BUZZER_PIN 32       // Buzzer pin
#define I2C_ADDRESS_1 0x48  // I2C address of ADS1115
#define I2C_ADDRESS_2  0x4A // I2C address of ADS1115 + SDA shorted to ADDR

CRGBArray<NUM_LEDS> leds;
ADS1115_WE adc_1 = ADS1115_WE(I2C_ADDRESS_1);
ADS1115_WE adc_2 = ADS1115_WE(I2C_ADDRESS_2);

float voltageArray[NUM_TRAYS];
const float threshold = 0.20;   //change as needed
volatile int trayFilled = 0;

const int debounceDelay = 50;
const int stayOpenTime = 10000;

unsigned long startOpenTime = 0;

enum State { // add more as needed
  idle,
  bird
};

State currentState = idle;
State prevState = idle;

void IRAM_ATTR raiseButtonIsr() {
  static long lastDebounceTime = 0;
  static bool isPressed = false;
  if (digitalRead(raiseButton) == LOW) {
    isPressed = true;
    lastDebounceTime = millis();
  } else {
    if (isPressed && (millis() - lastDebounceTime) > debounceDelay) {
      isPressed = false;
      raiseCurtain = true;
      lowerCurtain = false;
      startOpenTime = millis();
    }
  }
}

void startLowerCurtain() {
  if (!isLoweringCurtain) {
    myservo.write(0);
    Serial.println("Lowering!");
    isLoweringCurtain = true;
  }
}

void stopLowerCurtain() {
  isLoweringCurtain = false;
  isRaisingCurtain = false;
  isOpen = 0;
  myservo.write(90);
  Serial.println("Stopping!");
}

void stopRaiseCurtain() {
  isLoweringCurtain = false;
  isRaisingCurtain = false;
  isOpen = 1;
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

void checkCurtains() {
  if (currentState == State::bird && prevState == State::idle) {
    Serial.println("Lowering!");
    lowerCurtain = true;
    isOpen = 2;
    raiseCurtain = false;
  } else if (currentState == State::idle && prevState == State::bird) {
    Serial.println("Raising!");
    raiseCurtain = true;
    isOpen = 2;
    lowerCurtain = false;
  }

  if (isOpen == 1 && currentState == State::bird) {
    if (millis() - startOpenTime > stayOpenTime) {
      Serial.println("Timeout, lowering!");
      lowerCurtain = true;
      isOpen = 2;
      raiseCurtain = false;
    }
  }

  if (lowerCurtain && digitalRead(bottomDetectorPin)) {
    startLowerCurtain();
  } else if (lowerCurtain && !digitalRead(bottomDetectorPin)) {
    lowerCurtain = 0;
    stopLowerCurtain();
  }

  if (raiseCurtain && !digitalRead(topDetectorPin)) {
    startRaiseCurtain();
  } else if (raiseCurtain && digitalRead(topDetectorPin)) {
    raiseCurtain = 0;
    stopRaiseCurtain();
  }
}

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Wire.begin();
  Serial.begin(115200);
  ADSsetup();
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
  pinMode(raiseButton, INPUT_PULLUP);
  // pinMode(lowerButton, INPUT_PULLUP);
  attachInterrupt(raiseButton, raiseButtonIsr, CHANGE);

  // distance detection sensors
  pinMode(bottomDetectorPin, INPUT);
  pinMode(topDetectorPin, INPUT);
  // WIFIsetup();
}

void ADSsetup() {
  if (!adc_1.init()) { Serial.println("ADS1115_1 not connected!"); }
  if (!adc_2.init()) { Serial.println("ADS1115_2 not connected!"); }
  adc_1.setVoltageRange_mV(ADS1115_RANGE_6144);     // Set voltage range
  adc_1.setMeasureMode(ADS1115_CONTINUOUS);         // Set continuous measure mode
  adc_2.setVoltageRange_mV(ADS1115_RANGE_6144);     // Set voltage range
  adc_2.setMeasureMode(ADS1115_CONTINUOUS);         // Set continuous measure mode
}

void WIFIsetup() {
  mqttClient.setURI(server);
  mqttClient.enableLastWillMessage("lwt", "I am going offline");
  mqttClient.setKeepAlive(30);
  WiFi.begin(ssid, pass);
  WiFi.setHostname("esp32-sensors-actuators");
  mqttClient.loopStart();
  while (!mqttClient.isConnected()) {};
}

void sendMQTT() {
  if (mqttClient.isConnected()) {
    String trayFilledStr = String(trayFilled);
    mqttClient.publish(publishTopic, trayFilledStr, 0, false);
    Serial.println("MQTT message sent!");
  }
}

void onMqttConnect(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client))
  {
    mqttClient.subscribe(subscribeTopic, [](const String &payload) {
      Serial.println(payload);
      if (payload == "bird") {
        currentState = State::bird;
      } else if (payload == "idle") {
        currentState = State::idle;
      } 
    });
  }
}

void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
  mqttClient.onEventCallback(event);
}

void stateCheck() {
  ledModule();
  buzzerModule();
}

void loop() {
  readFSRs();
  readSerialInput();
  stateCheck();
  checkCurtains();
  unsigned long currentMillis = millis();
  // Check if the interval has passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendMQTT();
  }
}
void ledModule() {  //TODO: make blink fast
  static uint8_t hue = 0;
  uint8_t brightness = (currentState == State::bird) ? 100 : 0; // Set brightness based on state
  leds.fill_solid(CHSV(0, 255, brightness));
  FastLED.show();
}

void buzzerModule() {
  if (currentState == State::bird) {
    for (int i = 0; i < 5; i++) {
      TimerFreeTone(BUZZER_PIN, 2200, 500);  // Tone at pin, hz, duration(ms)
      delay(50);                             // Small delay between tones
    }
  }
}

float readchannel(ADS1115_MUX channel, ADS1115_WE adc_num) {
  adc_num.setCompareChannels(channel);
  return adc_num.getResult_V();
}

void readFSRs() {
  voltageArray[0] = readchannel(ADS1115_COMP_0_GND, adc_1);
  voltageArray[1] = readchannel(ADS1115_COMP_1_GND, adc_1);
  voltageArray[2] = readchannel(ADS1115_COMP_2_GND, adc_1);
  voltageArray[3] = readchannel(ADS1115_COMP_3_GND, adc_1);
  voltageArray[4] = readchannel(ADS1115_COMP_0_GND, adc_2);
  voltageArray[5] = readchannel(ADS1115_COMP_1_GND, adc_2);
  voltageArray[6] = readchannel(ADS1115_COMP_2_GND, adc_2);
  voltageArray[7] = readchannel(ADS1115_COMP_3_GND, adc_2);

  for (int i = 0; i < NUM_TRAYS; i++) { 
    Serial.print(i);
    Serial.print(": ");
    Serial.println(voltageArray[i]);
    if (voltageArray[i] > threshold) {
      trayFilled++; // send to server 
    } 
  }
}

void readSerialInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read data from Serial
    if (input == "bird") {
      currentState = State::bird;
    } else {
      currentState = State::idle;
    }
  }
}
