#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include "RTC.h"
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "website.h"

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

/* ===================== WIFI / RTC / MODES ===================== */

char ssid[] = "Mechatronics W183";
char pass[] = "Mechatronics183";

WiFiUDP Udp;
NTPClient timeClient(Udp, "pool.ntp.org", -8 * 3600);
unsigned long lastSync = 0;
const unsigned long syncInterval = 600000; 

WiFiServer server(80);
int currentMode = 0; // 0: Clock, 1: Stopwatch, 2: Countdown, 3: Alarm Trig

// Mode Variables
unsigned long swStart = 0, swElapsed = 0;
bool swRunning = false;
long cdSeconds = 0;
unsigned long cdLastMillis = 0;
int alH = -1, alM = -1;
bool alEnabled = false;

/* ===================== DISPLAY FUNCTIONS ===================== */

void showDigit(Adafruit_PWMServoDriver &pwm, int digitIndex, int value) {
  if (value < 0 || value > 9) {
    Serial.print("showDigit: invalid value "); Serial.println(value);
    return;
  }
  for (int s = 0; s < 7; s++) {
    int pulse = DIGIT_TO_SEGMENT[value][s] ? SERVO_ON : SERVO_OFF;
    pwm.writeMicroseconds(SERVO_CHANNELS[digitIndex][s], pulse);
  }
}

void showTime(int hour, int minute){
  int displayLeft = hour;
  int h = hour % 24;
  if (h < 0) h += 24; 
  int displayHour = h % 12;
  if (currentMode == 0) {
    displayLeft = hour % 12;
    if (displayLeft == 0) displayLeft = 12;
  }
  if (displayHour < 10) {
    for (int s = 0; s < 7; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[0][s], SERVO_OFF);
    }
  } else {
    showDigit(pwmHours, 0, displayHour / 10);
  }

  showDigit(pwmHours, 1, displayHour % 10);
  showDigit(pwmMinutes, 0, minute / 10);
  showDigit(pwmMinutes, 1, minute % 10);
}

void wiggle() {
  for(int i=0; i<7; i++) pwmHours.writeMicroseconds(SERVO_CHANNELS[0][i], SERVO_ON);
  delay(200);
  for(int i=0; i<7; i++) pwmHours.writeMicroseconds(SERVO_CHANNELS[0][i], SERVO_OFF);
  delay(200);
}

/* ===================== TIME SYNC ===================== */

void updateRTC() {
  timeClient.update();
  RTCTime newTime(timeClient.getEpochTime()); // Fixed lvalue error here
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
  server.begin();
  Serial.print("IP: "); Serial.println(WiFi.localIP());
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

  // Original startup sequence...
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(100);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(100);
    }
  }
  delay(1000);
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(100);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(100);
    }
  }
  delay(1000);

  connectToWiFi();
  RTC.begin();
  timeClient.begin();
  updateRTC();
}

/* ===================== LOOP ===================== */

void loop() {
  // Handle Web Interface
  WiFiClient client = server.available();
  if (client) {
    String req = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        req += c;
        if (c == '\n' && req.indexOf("\r\n\r\n") != -1) break;
      }
    }
    if (req.indexOf("GET /CLOCK") != -1) currentMode = 0;
    if (req.indexOf("GET /SW_START") != -1) { currentMode = 1; swStart = millis(); swRunning = true; }
    if (req.indexOf("GET /SW_STOP") != -1) { swElapsed += millis() - swStart; swRunning = false; }
    if (req.indexOf("GET /SW_RESET") != -1) { swElapsed = 0; swStart = millis(); }
    if (req.indexOf("GET /SET_CD") != -1) {
       int m = req.substring(req.indexOf("m=")+2, req.indexOf("&s=")).toInt();
       int s = req.substring(req.indexOf("s=")+2, req.indexOf(" ", req.indexOf("s="))).toInt();
       cdSeconds = (m * 60) + s; cdLastMillis = millis(); currentMode = 2;
    }
    if (req.indexOf("GET /SET_ALARM") != -1) {
       alH = req.substring(req.indexOf("atime=")+6, req.indexOf("atime=")+8).toInt();
       alM = req.substring(req.indexOf("atime=")+11, req.indexOf("atime=")+13).toInt();
       alEnabled = true;
    }
    if (req.indexOf("GET /OFF") != -1) { alEnabled = false; currentMode = 0; }

    client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    client.print(INDEX_HTML);
    client.stop();
  }

  // Handle Logic
  static int lastValL = -1, lastValR = -1;

  // Periodic NTP resync
  if (millis() - lastSync > syncInterval) {
    updateRTC();
    lastSync = millis();
  }

  RTCTime now;
  RTC.getTime(now);

  // Alarm Check
  if (alEnabled && now.getHour() == alH && now.getMinutes() == alM) currentMode = 3;

  if (currentMode == 0) { // Clock Mode
    if (now.getMinutes() != lastValR) {
      lastValR = now.getMinutes();
      showTime(now.getHour(), now.getMinutes());
    }
  } 

  // --- STOPWATCH LOGIC ---
  else if (currentMode == 1) { 
    unsigned long totalMillis = swElapsed + (swRunning ? (millis() - swStart) : 0);
    int totalSeconds = totalMillis / 1000;
    int s = totalSeconds % 60;
    int m = (totalSeconds / 60) % 100;

    if (s != lastValR) { // Only update if the second actually changed
      lastValR = s; 
      showTime(m, s); 
    }
  }
  
  // --- COUNTDOWN LOGIC ---
  else if (currentMode == 2) { 
    if (cdSeconds > 0) {
      if (millis() - cdLastMillis >= 1000) {
        cdSeconds--;
        cdLastMillis = millis();
        showTime(cdSeconds / 60, cdSeconds % 60);
      }
    } else {
      currentMode = 3; // Trigger Alarm wiggle
      wiggle();
    }
  }

  delay(100);
}
