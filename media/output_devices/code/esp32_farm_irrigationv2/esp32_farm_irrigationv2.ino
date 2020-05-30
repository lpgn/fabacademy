//libraries
#include <virtuabotixRTC.h>

//Settings
virtuabotixRTC myRTC(5, 18, 19);// (CLK,DAT,RST)
int pumprelayPin1 = 22;

//defining variables for timer
int relay1houron = 16;
int relay1houroff = 17;
int relay1minon = 16;
int relay1minoff = 17;

//run once
void setup() {

   Serial.begin(115200);
   Serial.println("Program started");
myRTC.setDS1302Time(50, 15, 16, 6, 24, 5, 2020); // sets seconds, minutes, hours (24h mode), day of the week, day, month, year) comment after first compile
  
  pinMode(pumprelayPin1, OUTPUT);
  
}

//run in loop
void loop() {
  myRTC.updateTime();//updates time
  Serial.print("Current Time: ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes);
  Serial.print(":");
  Serial.print(myRTC.seconds);
  Serial.println();
  delay (1000);
//condition (logic test)
if ((myRTC.hours) == relay1houron && (myRTC.minutes) == relay1minon) {
  digitalWrite(pumprelayPin1, HIGH);
  Serial.println("Pump is on");
  }  
if ((myRTC.hours) == relay1houroff && (myRTC.minutes) == relay1minoff) {
  digitalWrite(pumprelayPin1, LOW);
  Serial.println("Pump is off");
  }  
}
