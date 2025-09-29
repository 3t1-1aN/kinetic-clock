#include <Servo.h>
#include "RTC.h"
#include <NTPClient.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

const int SERVO_PINS[7] = {6, 7, 8, 9, 10, 11, 12};
Servo servos[7];
const int DIGIT_TO_SEGMENT[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, //0
  {0, 1, 1, 0, 0, 0, 0}, //1
  {1, 1, 0, 1, 1, 0, 1}, //2
  {1, 1, 1, 1, 0, 0, 1}, //3
  {0, 1, 1, 0, 0, 1, 1}, //4
  {1, 0, 1, 1, 0, 1, 1}, //5
  {1, 0, 1, 1, 1, 1, 1}, //6
  {1, 1, 1, 0, 0, 0, 0}, //7
  {1, 1, 1, 1, 1, 1, 1}, //8
  {1, 1, 1, 1, 0, 1, 1}  //9
} ;

int SERVO_OFF = 160;
int SERVO_ON = 40;

char ssid[] = "Mechatronics W183"; 
char pass[] = "Mechatronics183";

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP
NTPClient timeClient(Udp);

void connectToWiFi(){
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifiStatus = WiFi.begin(ssid, pass);
    delay(10000);
  }

  Serial.println("Connected to WiFi");
}

void setup() {
  // put your setup code here, to run once:
/* WIFI */
  Serial.begin(9600);
  while (!Serial);

  connectToWiFi();
  RTC.begin();
  Serial.println("\nStarting connection to server...");
  timeClient.begin();
  timeClient.update();
  updateRTC(); // initial sync at startup
  lastSync = millis();
  auto timeZoneOffsetHours = 2;
  auto unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  RTCTime currentTime;
  RTC.getTime(currentTime); 
  Serial.println("The RTC was just set to: " + String(currentTime));

/* SERVO MOTORS */
  for (int i=0; i<7; i++) {
    servos[i].attach(SERVO_PINS[i]);
    servos[i].write(SERVO_OFF);
  }
}

void showDigits(int digit) {
  for (int s=0; s<7; s++) {
    int state = DIGIT_TO_SEGMENT[digit][s];
    if (state == 1) {
      servos[s].write(SERVO_ON);
    } 
    else {
      servos[s].write(SERVO_OFF);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
/* WIFI */
  unsigned long now = millis();

  // Update RTC every 1 hour
  if (now - lastSync >= syncInterval) {
    updateRTC();
    lastSync = now;
  }

  // Print current RTC time every second
  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.println("RTC: " + String(currentTime));

  delay(1000);
/* SERVO MOTORS */
  for (int d=0; d<=9; d++) {
  showDigits(d) ;
  delay(500);
  }
}
