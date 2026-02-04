#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include "RTC.h"
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

/* ===================== DISPLAY CONFIG ===================== */

#define NUM_SEGMENTS 7
#define NUM_DIGITS 2

Adafruit_PWMServoDriver pwmHours(0x40);
Adafruit_PWMServoDriver pwmMinutes(0x41);

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

void showDigit(Adafruit_PWMServoDriver &pwm, int digitIndex, int value) {
  // Safety guard: prevents bus faults
  if (value < 0 || value > 9) {
    Serial.print("Invalid digit value: ");
    Serial.println(value);
    return;
  }

  for (int s = 0; s < NUM_SEGMENTS; s++) {
    int pulse = DIGIT_TO_SEGMENT[value][s] ? SERVO_ON : SERVO_OFF;
    pwm.writeMicroseconds(SERVO_CHANNELS[digitIndex][s], pulse);
    delay(50);
  }
}

void showTime(int hour, int minute) {
  // Normalize hour safely
  int h = hour % 24;
  if (h < 0) h += 24;

  // Convert to 12-hour display
  int displayHour = h % 12;
  if (displayHour == 0) displayHour = 12;

  showDigit(pwmHours, 0, displayHour / 10);
  showDigit(pwmHours, 1, displayHour % 10);

  showDigit(pwmMinutes, 0, minute / 10);
  showDigit(pwmMinutes, 1, minute % 10);
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

  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(3000, true);

  pwmHours.begin();
  pwmMinutes.begin();
  pwmHours.setPWMFreq(50);
  pwmMinutes.setPWMFreq(50);

  // Initialize servos ON
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(100);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(100);
    }
  }
  delay(1000);

  // Initialize servos OFF
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(100);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(100);
    }
  }
  delay(1000);

  Serial.println("Clock initialized");

  // WiFi + RTC
  connectToWiFi();
  RTC.begin();
  timeClient.begin();
  updateRTC();
}

/* ===================== LOOP ===================== */

void loop() {
  static int lastMinute = -1;

  // Periodic NTP resync
  if (millis() - lastSync > syncInterval) {
    updateRTC();
    lastSync = millis();
  }

  RTCTime now;
  RTC.getTime(now);

  int hour = now.getHour();     // 0–23
  int minute = now.getMinutes();

  if (minute != lastMinute) {
    lastMinute = minute;

    Serial.print("Displaying time: ");
    Serial.print(hour);
    Serial.print(":");
    Serial.println(minute);

    showTime(hour, minute);
  }

  delay(500);
}
