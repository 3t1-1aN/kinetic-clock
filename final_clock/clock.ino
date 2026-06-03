#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>

#include "RTC.h"
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "website.h"
#include <TimeLib.h>
#include <Timezone.h>

/* ===================== DISPLAY CONFIG ===================== */

#define NUM_SEGMENTS 7
#define NUM_DIGITS 2

// Delay (ms) between each individual servo write — keeps peak current low.
// 7 segments × 4 digits × 20ms = 560ms max for a full display update.
// Tune this value up/down to balance speed vs. current draw.
#define SERVO_STAGGER_MS 10

TimeChangeRule pdtRule = {"PDT", Second, Sun, Mar, 2, -420}; // DST begins
TimeChangeRule pstRule = {"PST", First, Sun, Nov, 2, -480};  // Standard time
Timezone pacific(pdtRule, pstRule);

Adafruit_PWMServoDriver pwmHours(0x40);
Adafruit_PWMServoDriver pwmMinutes(0x41);

const int SERVO_CHANNELS[NUM_DIGITS][NUM_SEGMENTS] = {
  {0, 1, 2, 3, 4, 5, 6},
  {7, 8, 9, 10, 11, 12, 13}
};

const int SERVO_ON  = 1700;
const int SERVO_OFF = 2500;

static int lastClockH = -1;
static int lastClockM = -1;

int lastH10 = -99;
int lastH1  = -99;
int lastM10 = -99;
int lastM1  = -99;

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

/* ===================== NEOPIXEL CONFIG ===================== */

#define NEO_PIN    9
#define NEO_COUNT  23

Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

void initNeoPixels() {
  strip.begin();
  strip.setBrightness(180); // 0–255; tune to taste
  strip.fill(strip.Color(255, 255, 255)); // solid white
  strip.show();
}

/* ===================== WIFI / RTC / MODES ===================== */

char ssid[] = "Mechatronics W183";
char pass[] = "Mechatronics183";
//char ssid[] = "MetroEDStudent";
//char pass[] = "CareerTech";

WiFiUDP Udp;
NTPClient timeClient(Udp, "pool.ntp.org");
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

void resetDisplayCache() {
  lastH10 = -99;
  lastH1  = -99;
  lastM10 = -99;
  lastM1  = -99;

  lastClockH = -1;
  lastClockM = -1;
}

// Write a single servo and wait SERVO_STAGGER_MS before the next one.
// This spreads the inrush current across time instead of spiking all at once.
void writeServoStaggered(Adafruit_PWMServoDriver &pwm, int channel, int pulse) {
  pwm.writeMicroseconds(channel, pulse);
  delay(SERVO_STAGGER_MS);
}

void showDigit(Adafruit_PWMServoDriver &pwm, int digitIndex, int value) {
  if (value < 0 || value > 9) {
    Serial.print("showDigit: invalid value "); Serial.println(value);
    return;
  }
  for (int s = 0; s < 7; s++) {
    int pulse = DIGIT_TO_SEGMENT[value][s] ? SERVO_ON : SERVO_OFF;
    writeServoStaggered(pwm, SERVO_CHANNELS[digitIndex][s], pulse);
  }
}

// Blank a digit (all segments off) with staggering.
void blankDigit(Adafruit_PWMServoDriver &pwm, int digitIndex) {
  for (int s = 0; s < 7; s++) {
    writeServoStaggered(pwm, SERVO_CHANNELS[digitIndex][s], SERVO_OFF);
  }
}

void showTime(int hour, int minute) {
  int h = hour % 24;
  if (h < 0) h += 24;
  int displayHour = h % 12;
  if (currentMode == 0) {
    displayHour = hour % 12;
    if (displayHour == 0) displayHour = 12;
  }
  int h10 = displayHour / 10;
  int h1  = displayHour % 10;
  int m10 = minute / 10;
  int m1  = minute % 10;

  // ----- Hour Tens -----
  if (displayHour < 10) {
    // Blank only if not already blank
    if (lastH10 != -1) {
      blankDigit(pwmHours, 0);
      lastH10 = -1;
    }
  } else {
    if (h10 != lastH10) {
      showDigit(pwmHours, 0, h10);
      lastH10 = h10;
    }
  }

  // ----- Hour Ones -----
  if (h1 != lastH1) {
    showDigit(pwmHours, 1, h1);
    lastH1 = h1;
  }

  // ----- Minute Tens -----
  if (m10 != lastM10) {
    showDigit(pwmMinutes, 0, m10);
    lastM10 = m10;
  }

  // ----- Minute Ones -----
  if (m1 != lastM1) {
    showDigit(pwmMinutes, 1, m1);
    lastM1 = m1;
  }
}

void wiggle() {
  for(int i=0; i<7; i++) {
    pwmHours.writeMicroseconds(SERVO_CHANNELS[0][i], SERVO_ON);
    delay(SERVO_STAGGER_MS);
  }
  delay(200);
  for(int i=0; i<7; i++) {
    pwmHours.writeMicroseconds(SERVO_CHANNELS[0][i], SERVO_OFF);
    delay(SERVO_STAGGER_MS);
  }
  delay(200);
}

void slotMachineAnimation() {

  Serial.println("Starting slot machine animation");

  int h10 = 0;
  int h1  = 0;
  int m10 = 0;
  int m1  = 0;

  // Spin digits — stagger is already inside showDigit
  for (int i = 0; i < 15; i++) {

    h10 = random(0, 10);
    h1  = random(0, 10);
    m10 = random(0, 6);
    m1  = random(0, 10);

    showDigit(pwmHours,   0, h10);
    showDigit(pwmHours,   1, h1);
    showDigit(pwmMinutes, 0, m10);
    showDigit(pwmMinutes, 1, m1);

    delay(120);
  }

  // Reel stop 1
  h10 = random(0,10);
  showDigit(pwmHours,0,h10);
  delay(300);

  // Reel stop 2
  h1 = random(0,10);
  showDigit(pwmHours,1,h1);
  delay(300);

  // Reel stop 3
  m10 = random(0,6);
  showDigit(pwmMinutes,0,m10);
  delay(300);

  // Reel stop 4
  m1 = random(0,10);
  showDigit(pwmMinutes,1,m1);
  delay(500);

  // Final landing
  showTime(0,0);

  delay(400);

  wiggle();

  lastH10 = -99;
  lastH1  = -99;
  lastM10 = -99;
  lastM1  = -99;
}

/* ===================== TIME SYNC ===================== */

void updateRTC() {

  Serial.println("===== NTP SYNC =====");

  bool success = false;

  for (int i = 0; i < 5; i++) {
    success = timeClient.forceUpdate();

    if (success) {
      break;
    }

    delay(2000);
  }

  Serial.print("forceUpdate() = ");
  Serial.println(success);

  if (!success) {
    Serial.println("NTP update failed");
    return;
  }

  time_t utc = timeClient.getEpochTime();

  Serial.print("UTC Epoch = ");
  Serial.println(utc);

  if (utc < 1700000000UL) {
    Serial.println("Invalid epoch received");
    return;
  }

  // Convert UTC -> Pacific time (handles DST)
  time_t local = pacific.toLocal(utc);

  setTime(local);

  RTCTime rtcTime(local);
  RTC.setTime(rtcTime);

  Serial.println("RTC updated from NTP");

  RTCTime verify;
  RTC.getTime(verify);

  Serial.print("RTC now reads: ");
  Serial.print(verify.getHour());
  Serial.print(":");
  if (verify.getMinutes() < 10) Serial.print("0");
  Serial.print(verify.getMinutes());
  Serial.print(":");
  if (verify.getSeconds() < 10) Serial.print("0");
  Serial.println(verify.getSeconds());

  Serial.println("====================");

  resetDisplayCache();
}

bool wifiConnected = false;

void connectToWiFi() {
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 3) {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, pass);
    delay(8000);

    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());

    server.begin();
    wifiConnected = true;
  }
  else {
    Serial.println("WiFi FAILED → entering demo countdown mode");

    wifiConnected = false;

    currentMode = 4;
    cdSeconds = 120;
    cdLastMillis = millis();
  }
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

  // NeoPixels on before anything else so there's light during startup
  initNeoPixels();

  // Startup sequence — staggered to limit inrush current
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(SERVO_STAGGER_MS);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_ON);
      delay(SERVO_STAGGER_MS);
    }
  }
  delay(1000);
  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < NUM_SEGMENTS; s++) {
      pwmHours.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(SERVO_STAGGER_MS);
      pwmMinutes.writeMicroseconds(SERVO_CHANNELS[d][s], SERVO_OFF);
      delay(SERVO_STAGGER_MS);
    }
  }
  delay(1000);

  connectToWiFi();
  RTC.begin();
  timeClient.begin();
  timeClient.setUpdateInterval(60000);
  updateRTC();
  lastSync = millis();
}

/* ===================== LOOP ===================== */

void loop() {
  // Handle Web Interface
  if (wifiConnected) {

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

      if (req.indexOf("GET /CLOCK") != -1) {
        currentMode = 0; 
        resetDisplayCache();
      }
      if (req.indexOf("GET /SW_START") != -1) { currentMode = 1; resetDisplayCache(); swStart = millis(); swRunning = true; }
      if (req.indexOf("GET /SW_STOP") != -1) { swElapsed += millis() - swStart; swRunning = false; }
      if (req.indexOf("GET /SW_RESET") != -1) { 
        swElapsed = 0; 
        swStart = millis(); 
        resetDisplayCache();
      }

      if (req.indexOf("GET /SET_CD") != -1) {
        int m = req.substring(req.indexOf("m=")+2, req.indexOf("&s=")).toInt();
        int s = req.substring(req.indexOf("s=")+2, req.indexOf(" ", req.indexOf("s="))).toInt();
        cdSeconds = (m * 60) + s;
        cdLastMillis = millis();
        currentMode = 2;
        resetDisplayCache();
      }
      if (req.indexOf("GET /SET_ALARM") != -1) {
        int tIdx = req.indexOf("atime=");
          if (tIdx != -1) {
            String tVal = req.substring(tIdx + 6, tIdx + 11);
            alH = tVal.substring(0, 2).toInt();
            alM = tVal.substring(3, 5).toInt();
            alEnabled = true;
            Serial.print("Alarm set for ");
            Serial.print(alH);
            Serial.print(":");
            if (alM < 10) Serial.print("0");
            Serial.println(alM);
          }
    }
      if (req.indexOf("GET /OFF") != -1) {
        alEnabled = false;
        currentMode = 0;
        resetDisplayCache();
        Serial.println("Alarm disabled");
    }
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      client.print(INDEX_HTML);
      client.stop();
    }
  }

  // Handle Logic
  static int lastValL = -1, lastValR = -1;

  // Periodic NTP sync (every 10 minutes)
  if (wifiConnected && millis() - lastSync >= syncInterval) {
    updateRTC();
    lastSync = millis();
  }

  RTCTime now;
  RTC.getTime(now);

  // Alarm Check
  if (alEnabled && now.getHour() == alH && now.getMinutes() == alM) {
    currentMode = 3;
    alEnabled = false;
    Serial.println("ALARM TRIGGERED");
  }

  if (currentMode == 0) { // Clock Mode
    if (now.getHour() != lastClockH || now.getMinutes() != lastClockM) {
      lastClockH = now.getHour();
      lastClockM = now.getMinutes();
      showTime(now.getHour(), now.getMinutes());
    }
  } 

  // --- STOPWATCH LOGIC ---
  else if (currentMode == 1) { 
    unsigned long totalMillis = swElapsed + (swRunning ? (millis() - swStart) : 0);
    int totalSeconds = totalMillis / 1000;
    int s = totalSeconds % 60;
    int m = (totalSeconds / 60) % 100;

    if (s != lastValR) {
      lastValR = s; 
      showTime(m, s); 
    }
  }
  
  // --- COUNTDOWN LOGIC ---
  else if (currentMode == 2) {

    bool changed = false;

    while (millis() - cdLastMillis >= 1000 && cdSeconds > 0) {
      cdSeconds--;
      cdLastMillis += 1000;
      changed = true;
    }

    if (changed) {
      showTime(cdSeconds / 60, cdSeconds % 60);
    }

    if (cdSeconds <= 0) {
      currentMode = 3;
    }
  }

  else if (currentMode == 3) {
    slotMachineAnimation();
    resetDisplayCache();
    currentMode = 0;
  }

  // --- FALLBACK 2 MINUTE LOOP MODE ---
  else if (currentMode == 4) {

    bool changed = false;

    while (millis() - cdLastMillis >= 1000) {
      cdSeconds--;
      cdLastMillis += 1000;
      changed = true;
    }

    if (changed) {
      showTime(cdSeconds / 60, cdSeconds % 60);
    }

    if (cdSeconds <= 0) {
      Serial.println("Demo loop finished → wiggle");
      slotMachineAnimation();
      cdSeconds = 120;
      cdLastMillis = millis();
    }
  }

  delay(100);
}
