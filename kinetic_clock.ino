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
  for (int s = 0; s < 7; s++) {
    int pulse = DIGIT_TO_SEGMENT[value][s] ? SERVO_ON : SERVO_OFF;
    pwm.writeMicroseconds(SERVO_CHANNELS[digitIndex][s], pulse);
    delay(30);
  }
}

void showTime(int hour, int minute){
  showDigit(pwmHours, 0, hour/10);
  showDigit(pwmHours, 1, hour%10);

  showDigit(pwmMinutes, 0, minute/10);
  showDigit(pwmMinutes, 1, minute%10);
}

void showNumber(int number) {
  int tens = (number / 10) % 10;
  int ones = number % 10;
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

  pwmHours.begin();
  pwmMinutes.begin();
  pwmHours.setPWMFreq(50);
  pwmMinutes.setPWMFreq(50);

  // Initialize servos ON
  for (int d = 0; d < 4; d++) {
    for (int s = 0; s < 7; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(100);
    }
  }

  // Initialize servos OFF
  for (int d = 0; d < 4; d++) {
    for (int s = 0; s < 7; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(100);
    }
  }

  Serial.println("Clock initialized");
}

/* ===================== LOOP ===================== */

void loop() {
  static int lastHour = -1;
  static int lastMinute = -1;

  // Periodic NTP resync
  if (millis() - lastSync > syncInterval) {
    updateRTC();
    lastSync = millis();
  }

  RTCTime now;
  RTC.getTime(now);

  int hour = now.getHour() - 12;
  int minute = now.getMinutes();

    if (minute != lastMinute) {
      lastMinute = minute;

      Serial.print("Displaying minutes: ");
      Serial.println(minute);

      showTime(hour, minute);
    }

  delay(500);
}
