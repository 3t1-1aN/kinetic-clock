#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "RTC.h"

char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASSWORD";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // sync every 60s

void setup() {
  Serial.begin(115200);

  // connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // start NTP
  timeClient.begin();

  // sync RTC once at startup
  updateRTC();
}

void loop() {
  static unsigned long lastSync = 0;

  if (millis() - lastSync > 60000) { // every minute
    updateRTC();
    lastSync = millis();
  }

  // read RTC for "always-on" time
  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.println(currentTime.toString());
  delay(1000);
}

void updateRTC() {
  if (timeClient.update()) {
    unsigned long epoch = timeClient.getEpochTime();
    RTCTime newTime(epoch);
    RTC.setTime(newTime);
    Serial.println("RTC synced!");
  }
}
