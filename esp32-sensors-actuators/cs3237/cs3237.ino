#include <Arduino.h>
#include <ADS1115_WE.h>
#include <Wire.h>
#include <FastLED.h>
#include <TimerFreeTone.h>
#include <WiFi.h>
#include "ESP32MQTTClient.h"
#include <ESP32Servo.h>

#define NUM_LEDS          4         // Number of LEDs on the strip
#define NUM_TRAYS         8         // Number of trays
#define DATA_PIN          33        // LED data pin
#define BUZZER_PIN        32        // Buzzer pin
#define I2C_ADDRESS_1     0x48      // I2C address of ADS1115
#define I2C_ADDRESS_2     0x4A      // I2C address of ADS1115 + SDA shorted to ADDR
#define SERVO_PIN         2
#define BOTTOM_DETECT_PIN 13
#define TOP_DETECT_PIN    12
#define BUTTON_PIN        14

enum State { // add more as needed
  idle,
  bird
};
State currentState = idle;
State prevState = idle;

CRGBArray<NUM_LEDS> leds;
ADS1115_WE adc_1 = ADS1115_WE(I2C_ADDRESS_1);
ADS1115_WE adc_2 = ADS1115_WE(I2C_ADDRESS_2);

const char *ssid = "Galaxy S21+ 5Gf502";
const char *pass = "fmkz9731";

char *server = "mqtt://192.168.230.245:1883";

char *subscribeTopic = "tray-return/actuate"; // bird detected
char *publishTopic = "tray-return/sensor"; 

unsigned long previousMillis = 0;    // Stores the last time the function was called
const long interval = 60000;

ESP32MQTTClient mqttClient;
Servo myservo;  // create servo object to control a servo

int pos = 0;    // variable to store the servo position
bool lowerCurtain = false;
bool raiseCurtain = false;
bool isLoweringCurtain = false;
bool isRaisingCurtain = false;
int isOpen = 2;

float voltageArray[NUM_TRAYS];
const float threshold = 0.20;   //change as needed
volatile int trayFilled = 0;

const int debounceDelay = 50;
const int stayOpenTime = 10000;

unsigned long startOpenTime = 0;

void IRAM_ATTR raiseButtonIsr() {
  static long lastDebounceTime = 0;
  static bool isPressed = false;
  if (digitalRead(BUTTON_PIN) == LOW) {
    isPressed = true;
    lastDebounceTime = millis();
  } else { // if high
    if (isPressed && (millis() - lastDebounceTime) > debounceDelay) {
      isPressed = false;
      raiseCurtain = true;
      lowerCurtain = false;
      startOpenTime = millis();
    }
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

  if (lowerCurtain && digitalRead(BOTTOM_DETECT_PIN)) {
    startLowerCurtain();
  } else if (lowerCurtain && !digitalRead(BOTTOM_DETECT_PIN)) {
    lowerCurtain = 0;
    stopLowerCurtain();
  }

  if (raiseCurtain && !digitalRead(TOP_DETECT_PIN)) {
    startRaiseCurtain();
  } else if (raiseCurtain && digitalRead(TOP_DETECT_PIN)) {
    raiseCurtain = 0;
    stopRaiseCurtain();
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

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Wire.begin();
  Serial.begin(115200);
  ADSsetup();
  servo_setup();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // pinMode(lowerButton, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, raiseButtonIsr, CHANGE);

  // distance detection sensors
  pinMode(BOTTOM_DETECT_PIN, INPUT);
  pinMode(TOP_DETECT_PIN, INPUT);
  WIFIsetup();
}

void servo_setup() {
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(SERVO_PIN, 1000, 2000); // attaches the servo on pin 2 to the servo object
  myservo.write(90);
  // distance detection sensors
  pinMode(BOTTOM_DETECT_PIN, INPUT);
  pinMode(TOP_DETECT_PIN, INPUT);
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

void stateCheck() {
  if (currentState == State::bird) {
    ledModule();
    buzzerModule();
  }
}

void ledModule() {  //TODO: make blink fast
  static uint8_t hue = 0;
  uint8_t brightness = (currentState == State::bird) ? 100 : 0; // Set brightness based on state
  leds.fill_solid(CHSV(0, 255, brightness));
  FastLED.show();
}

void buzzerModule() {
  TimerFreeTone(BUZZER_PIN, 2200, 500);  // Tone at pin, hz, duration(ms) 
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
  trayFilled = 0;
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
  else {
    prevState = currentState; 
  }
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
