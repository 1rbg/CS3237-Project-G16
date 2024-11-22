#include <WiFi.h>

const char *ssid = "[REDACTED]"; //wifi ssid here
const char *pass = "[REDACTED]"; //wifi password here

void wifiSetup() {
  WiFi.begin(ssid, pass);
  WiFi.setHostname("c3test");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
}