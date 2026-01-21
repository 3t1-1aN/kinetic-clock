#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include "RTC.h"
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

/* ===================== DISPLAY CONFIG ===================== */

#define NUM_SEGMENTS 7
#define NUM_DIGITS 2

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Digit 0 = tens, Digit 1 = ones
const int SERVO_CHANNELS[NUM_DIGITS][NUM_SEGMENTS] = {
  {0, 1, 2, 3, 4, 5, 6},
  {7, 8, 9, 10, 11, 12, 13}
};

const int SERVO_ON  = 1700;
const int SERVO_OFF = 2500;

const int DIGIT_TO_SEGMENT[10][NUM_SEGMENTS] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 0, 0, 0, 1, 1, 0}, // 1
  {1, 0, 1, 1, 0, 1, 1}, // 2
  {1, 0, 0, 1, 1, 1, 1}, // 3
  {0, 1, 0, 0, 1, 1, 1}, // 4
  {1, 1, 0, 1, 1, 0, 1}, // 5
  {1, 1, 1, 1, 1, 0, 1}, // 6
  {1, 0, 0, 0, 1, 1, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 0, 1, 1, 1, 1}  // 9
};

/* ===================== WIFI / RTC ===================== */

char ssid[] = "Mechatronics W183";
char pass[] = "Mechatronics183";

WiFiUDP Udp;
NTPClient timeClient(Udp, "pool.ntp.org", -8 * 3600);
unsigned long lastSync = 0;
const unsigned long syncInterval = 3600000; // 1 hour

/* ===================== DISPLAY FUNCTIONS ===================== */

void showDigit(int digitIndex, int value) {
  for (int s = 0; s < NUM_SEGMENTS; s++) {
    int pulse = DIGIT_TO_SEGMENT[value][s] ? SERVO_ON : SERVO_OFF;
    pwm.writeMicroseconds(SERVO_CHANNELS[digitIndex][s], pulse);
    delay(30);
  }
}

void showNumber(int number) {
  int tens = (number / 10) % 10;
  int ones = number % 10;

  showDigit(0, tens);
  showDigit(1, ones);
}

/* ===================== TIME SYNC ===================== */

void updateRTC() {
  timeClient.update();
  unsigned long unixTime = timeClient.getEpochTime();
  RTCTime newTime(unixTime);
  RTC.setTime(newTime);
  Serial.println("RTC updated from NTP");
}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("WiFi connected");
}

/* ===================== SETUP ===================== */

void setup() {
  Serial.begin(115200);
  delay(1000);

  // WiFi + RTC
  connectToWiFi();
  RTC.begin();
  timeClient.begin();
  updateRTC();

  // PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);

  // Initialize servos OFF
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwm.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
    }
  }

  Serial.println("Clock initialized");
}

/* ===================== LOOP ===================== */

void loop() {
  static int lastHour = -1;

  // Periodic NTP resync
  if (millis() - lastSync > syncInterval) {
    updateRTC();
    lastSync = millis();
  }

  RTCTime now;
  RTC.getTime(now);

  int hour = now.getHour(); // 0–23

  if (hour != lastHour) {
    lastHour = hour;

    Serial.print("Displaying hour: ");
    Serial.println(hour);

    showNumber(hour);
  }

  delay(500);
}
