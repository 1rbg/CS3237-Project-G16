#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include "ESP32MQTTClient.h"
#include "WiFisetup.h"
#include "base64.h"
// define the number of bytes you want to access

RTC_DATA_ATTR int bootCount = 0;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

int cameraImageExposure = 0;    // Camera exposure (0 - 1200)   If gain and exposure both set to zero then auto adjust is enabled
int cameraImageGain = 0;        // Image gain (0 - 30)
int cameraImageBrightness = 0;  // set (-2 to 2)

char *server = "mqtt://192.168.107.245:1883";

char *subscribeTopic = "esp/32";
char *publishTopic = "esp32/image";

ESP32MQTTClient mqttClient;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void deepSleepSetup() {
  print_wakeup_reason();
  //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1);
}

void mqttSetup() {
  mqttClient.enableDebuggingMessages();

  mqttClient.setURI(server);
  mqttClient.enableLastWillMessage("lwt", "I am going offline");
  mqttClient.setKeepAlive(30);
  mqttClient.loopStart();
}

void adjustCameraSettings() {
  sensor_t *s = esp_camera_sensor_get();

  // if both set to zero enable auto adjust
  if (cameraImageExposure == 0 && cameraImageGain == 0) {
    // enable auto adjust
    s->set_gain_ctrl(s, 1);                       // auto gain on
    s->set_exposure_ctrl(s, 1);                   // auto exposure on
    s->set_awb_gain(s, 1);                        // Auto White Balance enable (0 or 1)
    s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
  } else {
    // Apply manual settings
    s->set_gain_ctrl(s, 0);                       // auto gain off
    s->set_awb_gain(s, 1);                        // Auto White Balance enable (0 or 1)
    s->set_exposure_ctrl(s, 0);                   // auto exposure off
    s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
    s->set_agc_gain(s, cameraImageGain);          // set gain manually (0 - 30)
    s->set_aec_value(s, cameraImageExposure);     // set exposure manually  (0-1200)
    s->set_special_effect(s, 0);
  }
  
}

void setup() {

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable brownout detector
  Serial.begin(115200);

  Serial.setDebugOutput(true);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  pinMode(4, INPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_dis(GPIO_NUM_4);
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  print_wakeup_reason();
  deepSleepSetup();
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    rtc_gpio_hold_en(GPIO_NUM_4);
    delay(1000);
    esp_deep_sleep_start();
  }
  adjustCameraSettings();
  camera_fb_t *fb = NULL;
  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb) {
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    rtc_gpio_hold_en(GPIO_NUM_4);
    delay(1000);
    esp_deep_sleep_start();
  }
  for (int i = 0; i < 20; i++) {
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();
    if (!fb) {
      pinMode(4, OUTPUT);
      digitalWrite(4, LOW);
      rtc_gpio_hold_en(GPIO_NUM_4);
      delay(1000);
      esp_deep_sleep_start();
    }
  }
  String message = base64::encode(fb->buf, fb->len);

  Serial.println("i have taken picture.");
  wifiSetup();
  mqttSetup();

  while (mqttClient.isConnected() != true) {
    delay(10);
    Serial.println("connecting to mqtt server...");
  }
  //FAST MODE (increase MQTT_MAX_PACKET_SIZE)
  mqttClient.publish(publishTopic, message, 0, false);
  delay(1000);
  Serial.println("message published...");
  esp_camera_fb_return(fb);
  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4);



  Serial.println("Going to sleep now");
  delay(1000);
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop() {
}

void onMqttConnect(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client))  // can be omitted if only one client
  {
    mqttClient.subscribe(subscribeTopic, [](const String &payload) {
      Serial.println(String(subscribeTopic) + String(" ") + String(payload.c_str()));
    });

    mqttClient.subscribe("bar/#", [](const String &topic, const String &payload) {
      Serial.println(String(subscribeTopic) + String(" ") + String(payload.c_str()));
    });
  }
}

void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
  mqttClient.onEventCallback(event);
}