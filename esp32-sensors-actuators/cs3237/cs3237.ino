#include <Arduino.h>
#include <ADS1115_WE.h>
#include <Wire.h>
#include <FastLED.h>
#include <TimerFreeTone.h>
#include <WiFi.h>
#include "ESP32MQTTClient.h"

const char *ssid = "Galaxy S21+ 5Gf502";
const char *pass = "fmkz9731";

char *server = "mqtt://192.168.237.245:1883";

char *subscribeTopic = "tray-return/actuate";
char *publishTopic = "tray-return/sensor";

unsigned long previousMillis = 0;    // Stores the last time the function was called
const long interval = 5000;

ESP32MQTTClient mqttClient;

#define NUM_LEDS 4         // Number of LEDs on the strip
#define NUM_TRAYS 4        // Number of trays
#define DATA_PIN 33        // LED data pin
#define BUZZER_PIN 32      // Buzzer pin
#define I2C_ADDRESS 0x48   // I2C address of ADS1115

CRGBArray<NUM_LEDS> leds;
ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

float voltageArray[4];
const float threshold = 1.20;
volatile int trayFilled = 0;

enum State { // add more as needed
  idle,
  bird
};
State currentState = idle;

void ledModule() {
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

float readChannel(ADS1115_MUX channel) {
  adc.setCompareChannels(channel);
  return adc.getResult_V();
}

void readFSRs() {
  trayFilled = 0;
  voltageArray[0] = readChannel(ADS1115_COMP_0_GND);
  voltageArray[1] = readChannel(ADS1115_COMP_1_GND);
  voltageArray[2] = readChannel(ADS1115_COMP_2_GND);
  voltageArray[3] = readChannel(ADS1115_COMP_3_GND);
  delay(100);  // Adjust as necessary

  for (int i = 0; i < NUM_TRAYS; i++) { // can remove
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

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Wire.begin();
  Serial.begin(115200);
  if (!adc.init()) {
    Serial.println("ADS1115 not connected!");
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_6144);     // Set voltage range
  adc.setMeasureMode(ADS1115_CONTINUOUS);         // Set continuous measure mode

  mqttClient.setURI(server);
  mqttClient.enableLastWillMessage("lwt", "I am going offline");
  mqttClient.setKeepAlive(30);
  WiFi.begin(ssid, pass);
  WiFi.setHostname("esp32-sensors-actuators");
  mqttClient.loopStart();
}

void sendMQTT() {
  if (mqttClient.isConnected()) {
    String trayFilledStr = String(trayFilled);
    mqttClient.publish(subscribeTopic, trayFilledStr, 0, false);
    Serial.println("MQTT message sent!");
  }
}

void loop() {
  readFSRs();
  readSerialInput();
  buzzerModule();
  ledModule();
  unsigned long currentMillis = millis();

  // Check if the interval has passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendMQTT();
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