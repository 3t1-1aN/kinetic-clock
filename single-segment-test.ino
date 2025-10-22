#include <Servo.h>
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
int SERVO_ON = 30;

void setup() {
  // put your setup code here, to run once:
  for (int i=0; i<7; i++) {
    servos[i].attach(SERVO_PINS[i]);
    servos[i].write(SERVO_OFF);
  }
}

void showDigits(int digit) {
  for (int s=0; s<7; s++) {
    int state = DIGIT_TO_SEGMENT[digit][s];
    servos[s].write(DIGIT_TO_SEGMENT[digit][s] ? SERVO_ON : SERVO_OFF);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int d=0; d<=9; d++) {
  showDigits(d);
  delay(1000);
  }
}
