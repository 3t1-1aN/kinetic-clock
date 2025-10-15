#include <Servo.h>
#include "RTC.h"
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

const int SERVO_PINS[7] = {6, 7, 8, 9, 10, 11, 12};
Servo servos[7];

const int DIGIT_TO_SEGMENT[10][7] = {
  {1, 1, 1, 1, 1, 1, 0},
  {0, 1, 1, 0, 0, 0, 0},
  {1, 1, 0, 1, 1, 0, 1},
  {1, 1, 1, 1, 0, 0, 1},
  {0, 1, 1, 0, 0, 1, 1},
  {1, 0, 1, 1, 0, 1, 1},
  {1, 0, 1, 1, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 0, 1, 1}
};

const int SERVO_OFF = 160;
const int SERVO_ON = 40;

char ssid[] = "Mechatronics W183";
char pass[] = "Mechatronics183";

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;
NTPClient timeClient(Udp);

unsigned long lastSync = 0;
const unsigned long syncInterval = 3600000; // 1 hour

void updateRTC() {
  timeClient.update();
  unsigned long unixTime = timeClient.getEpochTime();
  RTCTime newTime = RTCTime(unixTime);
  RTC.setTime(newTime);
  Serial.println("RTC updated from NTP");
}

void connectToWiFi() {
  String fv = WiFi.firmwareVersion();
  if (fv != WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Connecting to SSID: ");
    Serial.println(ssid);
    wifiStatus = WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("Connected to WiFi");
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  connectToWiFi();
  RTC.begin();

  Serial.println("Starting NTP...");
  timeClient.begin();
  timeClient.update();
  updateRTC();

  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.print("RTC initialized.\n");

  for (int i = 0; i < 7; i++) {
    servos[i].attach(SERVO_PINS[i]);
    servos[i].write(SERVO_OFF);
  }
}

void showDigits(int digit) {
  for (int s = 0; s < 7; s++) {
    servos[s].write(DIGIT_TO_SEGMENT[digit][s] ? SERVO_ON : SERVO_OFF);
  }
}

void loop() {
  unsigned long now = millis();

  if (now - lastSync >= syncInterval) {
    updateRTC();
    lastSync = now;
  }

  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.print("RTC Hour: ");
  Serial.println(currentTime.getHour());

  for (int d = 0; d <= 9; d++) {
    showDigits(d);
    delay(500);
  }
}
