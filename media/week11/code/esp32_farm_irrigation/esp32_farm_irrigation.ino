#include <virtuabotixRTC.h>

virtuabotixRTC myRTC(5, 18, 19);// (CLK,DAT,RST)

int pumprelayPin1 = 22;

void setup() {

   Serial.begin(115200);
   
  // myRTC.setDS1302Time(00, 28, 16, 6, 23, 5, 2020); // sets seconds, minutes, hours (24h mode), day of the week, day, month, year) comment after first compile
  
  pinMode(pumprelayPin1, OUTPUT);

}

void loop() {
  myRTC.updateTime();//updates time
  Serial.print("Current Time: ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes)
  delay (60000);
  if ((myRTC.hours) == 15 || (myRTC.minutes) == 0) {digitalWrite(pumprelayPin1, HIGH)}
  if ((myRTC.hours) >= 15 || (myRTC.minutes) >= 1) {digitalWrite(pumprelayPin1, LOW)}
}
