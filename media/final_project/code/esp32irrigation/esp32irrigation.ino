/****************************************************************************
 *                                  Aknowledments                           * 
 *                                  by LucioPGN                             * 
 ****************************************************************************/
/*  Up to this date: 07th of June 2020 I don't consider myself a programer 
 *  so I need to stand on top of giants sholders for my programing projects:
 *  A Portion of this code was based on Rui Santos Code;
 *  A Portion of this code was based on Shakeels code for ESP8266 ;
 *  My contributions: 
 *    -so far I ported Shakeels code into ESP32;
 *    -separate functions from shakeels code into files;
 *    -separated functions from Rui Santos code into files.
 *  Things I still want to program for my final project:
 *    -simplify the code creating more functions;
 *    -try to separate the HTML part for a cleaner code;
 *    -Improve the appearance/Interface of the code
 *    -Add readings to HTML
 *    -Add a log of occurrences like over current
 *    -Add more safety for the equipment
 *    -Add a phone interface (APP)
 *    -Add function to set current time
 *    -Add renaming function to each relay so one can relate the relay to the area of interest or at least rename relays to actual areas of the farm.
 * 
 ****************************************************************************/
 
/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <Hash.h>
  #include <FS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <virtuabotixRTC.h> // DS1302 RTC module library
#include "theHtml.h"

// Set wait times for pump and valve relays activation/deactivation
unsigned long waitTimePumpOn = 5000; // wait time (ms) from relay activation to pump activation
unsigned long waitTimeValveOff = 1000; // wait time (ms) from pump deactivation to relay deactivation
// NodeMCU ESP8266 pump and relay GPIO pins 

const int pinValveRelay1 = 26;
const int pinValveRelay2 = 25;
const int pinValveRelay3 = 33;
const int pinPumpRelay = 32;

const int printReadFile = 0 ;

// RCT Pins CLK,DAT,RST
const int clkPin = 18;
const int datPin = 19;
const int rstPin = 21;

// set analog pin connected to the ACS712 current sensor
const int ACS712_sensor = 34; 

// Creation of the Real Time Clock Object
virtuabotixRTC myRTC(clkPin, datPin, rstPin); // Wiring of the DS1302 RTC (CLK,DAT,RST)

const char* PARAM_INT1 = "valveRelay1_OnHour";
const char* PARAM_INT2 = "valveRelay1_OnMin";
const char* PARAM_INT3 = "valveRelay1_OffHour";
const char* PARAM_INT4 = "valveRelay1_OffMin";

const char* PARAM_INT5 = "valveRelay2_OnHour";
const char* PARAM_INT6 = "valveRelay2_OnMin";
const char* PARAM_INT7 = "valveRelay2_OffHour";
const char* PARAM_INT8 = "valveRelay2_OffMin";

const char* PARAM_INT9 = "valveRelay3_OnHour";
const char* PARAM_INT10 = "valveRelay3_OnMin";
const char* PARAM_INT11 = "valveRelay3_OffHour";
const char* PARAM_INT12 = "valveRelay3_OffMin";

const char* PARAM_FLOAT1 = "LowCurrentLimit";
const char* PARAM_FLOAT2 = "HighCurrentLimit";

// ACS712 current sensor
// number of samples taken in a single shot. check this for understanding why https://arduino.stackexchange.com/questions/19787/esp8266-analog-read-interferes-with-wifi
#define NUMBER_OF_SAMPLES 200 
// Output sensitivity in mV per Amp
const int mVperAmp = 100; 

// ACS712 datasheet: scale factor is 185 for 5A module, 100 for 20A module and 66 for 30A module
float VRMSoffset = 0.0; //0.005; // set quiescent Vrms output voltage

// voltage offset at analog input with reference to ground when no signal applied to the sensor.
float AC_current; // measured AC current Irms value (Amps)
int count = 0; // initialise current limit count to zero
unsigned long MinTimePumpOperation = 0; // minimum time (ms) for pump operation (due to immediate low or high current)
unsigned long RTCtimeInterval = 5000;  // prints RTC time every interval (ms)
unsigned long RTCtimeNow;
unsigned long configTimeInterval = 30000; // interval in ms of printed configuration time
unsigned long configTimeNow = 0; // stores current value from millis()

bool valve_1_state = 0; // valve 1 state initialised to OFF
bool valve_2_state = 0; // valve 2 state initialised to OFF
bool valve_3_state = 0; // valve 3 state initialised to OFF
bool pump_state = 0; // pump state initialised to OFF
bool LowCurrentLimit_state = 0; // initialise low current limit state (0 = normal, 1 = low)
bool HighCurrentLimit_state = 0; // initialise high current limit state (0 = normal, 1 = high)

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "rato";
const char* password = "imakestuff";

const char* PARAM_STRING = "inputString";
const char* PARAM_INT = "inputInt";
const char* PARAM_FLOAT = "inputFloat";

// Declaring variables for valve relays and initialising to zero
int valveRelay1_OnHour = 0;
int valveRelay1_OnMin = 0;
int valveRelay1_OffHour = 0;
int valveRelay1_OffMin = 0;

int valveRelay2_OnHour = 0;
int valveRelay2_OnMin = 0;
int valveRelay2_OffHour = 0;
int valveRelay2_OffMin = 0;

int valveRelay3_OnHour = 0;
int valveRelay3_OnMin = 0;
int valveRelay3_OffHour = 0;
int valveRelay3_OffMin = 0;

// Declaring variables for lower and upper current limits for pump and initialising to zero
float LowCurrentLimit = 0; // set the minimum current threshold in Amps
float HighCurrentLimit = 0; // set the maximum current threshold in Amps

// HTML web page to handle input fields bellow:
// (valveRelay1_OnHour, valveRelay1_OnMin, valveRelay1_OffHour, valveRelay1_OffMin)
// (valveRelay1_OnHour, valveRelay1_OnMin, valveRelay1_OffHour, valveRelay1_OffMin)
// (valveRelay1_OnHour, valveRelay1_OnMin, valveRelay1_OffHour, valveRelay1_OffMin)
// (LowCurrentLimit,HighCurrentLimit)



void setup() {
  Serial.begin(9600);
  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  WiFi.mode(WIFI_AP); 

//wifi for overall WiFi configuration
//wifi.sta for station mode functions
//wifi.ap for wireless access point (WAP or simply AP) functions
//wifi.ap.dhcp for DHCP server control
//wifi.eventmon for wifi event monitor
//wifi.monitor for wifi monitor mode
  
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
        // GET valveRelay1_OnHour value on <ESP_IP>/get?valveRelay1_OnHour=<inputMessage>
    if (request->hasParam(PARAM_INT1)) {
      inputMessage = request->getParam(PARAM_INT1)->value();
      writeFile(SPIFFS, "/valveRelay1_OnHour.txt", inputMessage.c_str());
    }
    // GET valveRelay1_OnMin value on <ESP_IP>/get?valveRelay1_OnMin=<inputMessage>
    else if (request->hasParam(PARAM_INT2)) {
      inputMessage = request->getParam(PARAM_INT2)->value();
      writeFile(SPIFFS, "/valveRelay1_OnMin.txt", inputMessage.c_str());
    }
    // GET valveRelay1_OffHour value on <ESP_IP>/get?valveRelay1_OffHour=<inputMessage>
    else if (request->hasParam(PARAM_INT3)) {
      inputMessage = request->getParam(PARAM_INT3)->value();
      writeFile(SPIFFS, "/valveRelay1_OffHour.txt", inputMessage.c_str());
    }
    // GET valveRelay1_OffMin value on <ESP_IP>/get?valveRelay1_OffMin=<inputMessage>
    else if (request->hasParam(PARAM_INT4)) {
      inputMessage = request->getParam(PARAM_INT4)->value();
      writeFile(SPIFFS, "/valveRelay1_OffMin.txt", inputMessage.c_str());
    }

    // GET valveRelay2_OnHour value on <ESP_IP>/get?valveRelay2_OnHour=<inputMessage>
    else if (request->hasParam(PARAM_INT5)) {
      inputMessage = request->getParam(PARAM_INT5)->value();
      writeFile(SPIFFS, "/valveRelay2_OnHour.txt", inputMessage.c_str());
    }
    // GET valveRelay2_OnMin value on <ESP_IP>/get?valveRelay2_OnMin=<inputMessage>
    else if (request->hasParam(PARAM_INT6)) {
      inputMessage = request->getParam(PARAM_INT6)->value();
      writeFile(SPIFFS, "/valveRelay2_OnMin.txt", inputMessage.c_str());
    }
    // GET valveRelay2_OffHour value on <ESP_IP>/get?valveRelay2_OffHour=<inputMessage>
    else if (request->hasParam(PARAM_INT7)) {
      inputMessage = request->getParam(PARAM_INT7)->value();
      writeFile(SPIFFS, "/valveRelay2_OffHour.txt", inputMessage.c_str());
    }
    // GET valveRelay2_OffMin value on <ESP_IP>/get?valveRelay2_OffMin=<inputMessage>
    else if (request->hasParam(PARAM_INT8)) {
      inputMessage = request->getParam(PARAM_INT8)->value();
      writeFile(SPIFFS, "/valveRelay2_OffMin.txt", inputMessage.c_str());
    }

    // GET valveRelay3_OnHour value on <ESP_IP>/get?valveRelay3_OnHour=<inputMessage>
    else if (request->hasParam(PARAM_INT9)) {
      inputMessage = request->getParam(PARAM_INT9)->value();
      writeFile(SPIFFS, "/valveRelay3_OnHour.txt", inputMessage.c_str());
    }
    // GET valveRelay3_OnMin value on <ESP_IP>/get?valveRelay3_OnMin=<inputMessage>
    else if (request->hasParam(PARAM_INT10)) {
      inputMessage = request->getParam(PARAM_INT10)->value();
      writeFile(SPIFFS, "/valveRelay3_OnMin.txt", inputMessage.c_str());
    }
    // GET valveRelay3_OffHour value on <ESP_IP>/get?valveRelay3_OffHour=<inputMessage>
    else if (request->hasParam(PARAM_INT11)) {
      inputMessage = request->getParam(PARAM_INT11)->value();
      writeFile(SPIFFS, "/valveRelay3_OffHour.txt", inputMessage.c_str());
    }
    // GET valveRelay3_OffMin value on <ESP_IP>/get?valveRelay3_OffMin=<inputMessage>
    else if (request->hasParam(PARAM_INT12)) {
      inputMessage = request->getParam(PARAM_INT12)->value();
      writeFile(SPIFFS, "/valveRelay3_OffMin.txt", inputMessage.c_str());
    }
    
    // GET LowCurrentLimit value on <ESP_IP>/get?LowCurrentLimit=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT1)) {
      inputMessage = request->getParam(PARAM_FLOAT1)->value();
      writeFile(SPIFFS, "/LowCurrentLimit.txt", inputMessage.c_str());
    }
    // GET HighCurrentLimit value on <ESP_IP>/get?HighCurrentLimit=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT2)) {
      inputMessage = request->getParam(PARAM_FLOAT2)->value();
      writeFile(SPIFFS, "/HighCurrentLimit.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
  pinMode(pinValveRelay1, OUTPUT);
  pinMode(pinValveRelay2, OUTPUT);
  pinMode(pinValveRelay3, OUTPUT);
  pinMode(pinPumpRelay, OUTPUT);

  // set initial state of all valve and pump relays to OFF
  digitalWrite(pinValveRelay1, LOW);
  digitalWrite(pinValveRelay2, LOW);
  digitalWrite(pinValveRelay3, LOW);
  digitalWrite(pinPumpRelay, LOW);
  
  Serial.println();
  Serial.println("NodeMCU Irrigation Control System");
  Serial.println("********************************************************");
  
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  //myRTC.setDS1302Time(0, 31, 14, 4, 10, 6, 2020); // uncomment line, upload to reset RTC and then comment, upload.
  myRTC.updateTime(); //update of variables for time or accessing the individual elements.
  
  // Start printing elements as individuals                                                                 
  Serial.print("Preset RTC Time and Date: ");                                                                          
  Serial.print(myRTC.hours);                          
  Serial.print(":");                                                                                      
  Serial.print(myRTC.minutes);                                                                            
  Serial.print(":");                                                                                      
  Serial.print(myRTC.seconds);
  Serial.print(" ");
  Serial.print(myRTC.dayofmonth);                                                                         
  Serial.print("/");                                                                                      
  Serial.print(myRTC.month);                                                                              
  Serial.print("/");                                                                                      
  Serial.println(myRTC.year);
  
  // Read saved values on NodeMCU SPIFFS
  valveRelay1_OnHour = readFile(SPIFFS, "/valveRelay1_OnHour.txt").toInt();
  valveRelay1_OnMin = readFile(SPIFFS, "/valveRelay1_OnMin.txt").toInt();
  valveRelay1_OffHour = readFile(SPIFFS, "/valveRelay1_OffHour.txt").toInt();
  valveRelay1_OffMin = readFile(SPIFFS, "/valveRelay1_OffMin.txt").toInt();

  valveRelay2_OnHour = readFile(SPIFFS, "/valveRelay2_OnHour.txt").toInt();
  valveRelay2_OnMin = readFile(SPIFFS, "/valveRelay2_OnMin.txt").toInt();
  valveRelay2_OffHour = readFile(SPIFFS, "/valveRelay2_OffHour.txt").toInt();
  valveRelay2_OffMin = readFile(SPIFFS, "/valveRelay2_OffMin.txt").toInt();

  valveRelay3_OnHour = readFile(SPIFFS, "/valveRelay3_OnHour.txt").toInt();
  valveRelay3_OnMin = readFile(SPIFFS, "/valveRelay3_OnMin.txt").toInt();
  valveRelay3_OffHour = readFile(SPIFFS, "/valveRelay3_OffHour.txt").toInt();
  valveRelay3_OffMin = readFile(SPIFFS, "/valveRelay3_OffMin.txt").toInt();
  
  LowCurrentLimit = readFile(SPIFFS, "/LowCurrentLimit.txt").toFloat();
  HighCurrentLimit = readFile(SPIFFS, "/HighCurrentLimit.txt").toFloat();
  
  Serial.println("********************************************************");
  Serial.println("Saved User Configuration:");
  Serial.println("--------------------------------------------------------");
  Serial.println("RTC-based Timers (Hours:Minutes)");
  Serial.print("Valve Relay 1 ON: ");
  Serial.print(valveRelay1_OnHour);
  Serial.print(":");
  Serial.println(valveRelay1_OnMin);
  Serial.print("Valve Relay 1 OFF: ");
  Serial.print(valveRelay1_OffHour);
  Serial.print(":");
  Serial.println(valveRelay1_OffMin);
  Serial.print("Valve Relay 2 ON: ");
  Serial.print(valveRelay2_OnHour);
  Serial.print(":");
  Serial.println(valveRelay2_OnMin);
  Serial.print("Valve Relay 2 OFF: ");
  Serial.print(valveRelay2_OffHour);
  Serial.print(":");
  Serial.println(valveRelay2_OffMin);
  Serial.print("Valve Relay 3 ON: ");
  Serial.print(valveRelay3_OnHour);
  Serial.print(":");
  Serial.println(valveRelay3_OnMin);
  Serial.print("Valve Relay 3 OFF: ");
  Serial.print(valveRelay3_OffHour);
  Serial.print(":");
  Serial.println(valveRelay3_OffMin);
  Serial.println("--------------------------------------------------------");
  Serial.print("Low Current Limit: ");
  Serial.print(LowCurrentLimit, 3);
  Serial.println(" Amps");
  Serial.print("High Current Limit: ");
  Serial.print(HighCurrentLimit, 3);
  Serial.println(" Amps");
  Serial.println("********************************************************");
}

void loop() {

  if (millis() - configTimeNow >= configTimeInterval)  // non-blocking, prints every 10s.
  {
  configTimeNow = millis();
  // Read saved values on NodeMCU SPIFFS
  valveRelay1_OnHour = readFile(SPIFFS, "/valveRelay1_OnHour.txt").toInt();
  valveRelay1_OnMin = readFile(SPIFFS, "/valveRelay1_OnMin.txt").toInt();
  valveRelay1_OffHour = readFile(SPIFFS, "/valveRelay1_OffHour.txt").toInt();
  valveRelay1_OffMin = readFile(SPIFFS, "/valveRelay1_OffMin.txt").toInt();

  valveRelay2_OnHour = readFile(SPIFFS, "/valveRelay2_OnHour.txt").toInt();
  valveRelay2_OnMin = readFile(SPIFFS, "/valveRelay2_OnMin.txt").toInt();
  valveRelay2_OffHour = readFile(SPIFFS, "/valveRelay2_OffHour.txt").toInt();
  valveRelay2_OffMin = readFile(SPIFFS, "/valveRelay2_OffMin.txt").toInt();

  valveRelay3_OnHour = readFile(SPIFFS, "/valveRelay3_OnHour.txt").toInt();
  valveRelay3_OnMin = readFile(SPIFFS, "/valveRelay3_OnMin.txt").toInt();
  valveRelay3_OffHour = readFile(SPIFFS, "/valveRelay3_OffHour.txt").toInt();
  valveRelay3_OffMin = readFile(SPIFFS, "/valveRelay3_OffMin.txt").toInt();
  
  LowCurrentLimit = readFile(SPIFFS, "/LowCurrentLimit.txt").toFloat();
  HighCurrentLimit = readFile(SPIFFS, "/HighCurrentLimit.txt").toFloat();
  
  Serial.println("********************************************************");
  Serial.println("Current User Configuration:");
  Serial.println("--------------------------------------------------------");
  Serial.println("RTC-based Timers (Hours:Minutes)");
  Serial.print("Valve Relay 1 ON: ");
  Serial.print(valveRelay1_OnHour);
  Serial.print(":");
  Serial.println(valveRelay1_OnMin);
  Serial.print("Valve Relay 1 OFF: ");
  Serial.print(valveRelay1_OffHour);
  Serial.print(":");
  Serial.println(valveRelay1_OffMin);
  Serial.print("Valve Relay 2 ON: ");
  Serial.print(valveRelay2_OnHour);
  Serial.print(":");
  Serial.println(valveRelay2_OnMin);
  Serial.print("Valve Relay 2 OFF: ");
  Serial.print(valveRelay2_OffHour);
  Serial.print(":");
  Serial.println(valveRelay2_OffMin);
  Serial.print("Valve Relay 3 ON: ");
  Serial.print(valveRelay3_OnHour);
  Serial.print(":");
  Serial.println(valveRelay3_OnMin);
  Serial.print("Valve Relay 3 OFF: ");
  Serial.print(valveRelay3_OffHour);
  Serial.print(":");
  Serial.println(valveRelay3_OffMin);
  Serial.println("--------------------------------------------------------");
  Serial.print("Low Current Limit: ");
  Serial.print(LowCurrentLimit, 3);
  Serial.println(" Amps");
  Serial.print("High Current Limit: ");
  Serial.print(HighCurrentLimit, 3);
  Serial.println(" Amps");
  Serial.println("********************************************************");
  }
  
  myRTC.updateTime(); // update of variables for time or accessing the individual elements.
  
  //------------------------------------------------------------------------------------
  // Valve Relay 1 timer control
  //------------------------------------------------------------------------------------
  
  // check valve status and timer to turn Valve Relay 1 ON
  if (valve_1_state == 0)
  {
    if (myRTC.hours == valveRelay1_OnHour && myRTC.minutes == valveRelay1_OnMin && myRTC.seconds < ((waitTimePumpOn + waitTimeValveOff  + MinTimePumpOperation) / 1000))
    {
    Serial.print("Valve Relay 1 Activation sequence RTC time: ");
    Serial.print(myRTC.hours);  // display the current hour from RTC module                
    Serial.print(":");                                                                                      
    Serial.print(myRTC.minutes);  // display the current minutes from RTC module            
    //Serial.print(":");                                                                                           
    //Serial.println(myRTC.seconds);  // display the seconds from RTC module
    digitalWrite(pinValveRelay1, HIGH);
    valve_1_state = 1;
    Serial.println("*** Valve Relay 1 turned ON ***");
    // wait then turn pump relay ON
    Serial.print("Waiting ");
    Serial.print(waitTimePumpOn / 1000);
    Serial.println("s before activating Pump Relay.");
    delay(waitTimePumpOn);
    digitalWrite(pinPumpRelay, HIGH);
    pump_state = 1;
    Serial.println("*** Pump Relay turned ON ***");
    }
  }
  
  // check valve status and timer to turn Valve Relay 1 OFF
  if (valve_1_state == 1)
  {
    if (myRTC.hours == valveRelay1_OffHour && myRTC.minutes == valveRelay1_OffMin)
    {
    Serial.print("Valve Relay 1 Deactivation sequence RTC time: ");
    Serial.print(myRTC.hours);  // display the current hour from RTC module                
    Serial.print(":");                                                                                      
    Serial.print(myRTC.minutes);  // display the current minutes from RTC module            
    //Serial.print(":");                                                                                           
    //Serial.println(myRTC.seconds);  // display the seconds from RTC module
    digitalWrite(pinPumpRelay, LOW);
    pump_state = 0;
    Serial.println("*** Pump Relay turned OFF ***");
    valve_1_state = turnOffRelay (pinValveRelay1, 1);
    }
  }
  
  //------------------------------------------------------------------------------------
  // Valve Relay 2 timer control
  //------------------------------------------------------------------------------------
  
  // check valve status and timer to turn Valve Relay 2 ON
  if (valve_2_state == 0)
  {
    if (myRTC.hours == valveRelay2_OnHour && myRTC.minutes == valveRelay2_OnMin && myRTC.seconds < ((waitTimePumpOn + waitTimeValveOff + MinTimePumpOperation) / 1000))
    {
    Serial.print("Valve Relay 2 Activation sequence RTC time: ");
    Serial.print(myRTC.hours);  // display the current hour from RTC module                
    Serial.print(":");                                                                                      
    Serial.print(myRTC.minutes);  // display the current minutes from RTC module            
    //Serial.print(":");                                                                                           
    //Serial.println(myRTC.seconds);  // display the seconds from RTC module  
    digitalWrite(pinValveRelay2, HIGH);
    valve_2_state = 1;
    Serial.println("*** Valve Relay 2 turned ON ***");
    // wait then turn pump relay ON
    Serial.print("Waiting ");
    Serial.print(waitTimePumpOn / 1000);
    Serial.println("s before activating Pump Relay.");
    delay(waitTimePumpOn);
    digitalWrite(pinPumpRelay, HIGH);
    pump_state = 1;
    Serial.println("*** Pump Relay turned ON ***");
    }
  }
  
  // check valve status and timer to turn Valve Relay 2 OFF
  if (valve_2_state == 1)
  {
    if (myRTC.hours == valveRelay2_OffHour && myRTC.minutes == valveRelay2_OffMin)
    {
    Serial.print("Valve Relay 2 Deactivation sequence RTC time: ");
    Serial.print(myRTC.hours);  // display the current hour from RTC module                
    Serial.print(":");                                                                                      
    Serial.print(myRTC.minutes);  // display the current minutes from RTC module            
    //Serial.print(":");                                                                                           
    //Serial.println(myRTC.seconds);  // display the seconds from RTC module
    digitalWrite(pinPumpRelay, LOW);
    pump_state = 0;
    Serial.println("*** Pump Relay turned OFF ***");
    valve_2_state = turnOffRelay (pinValveRelay2, 2);;
    }
  }

  //------------------------------------------------------------------------------------
  // Valve Relay 3 timer control
  //------------------------------------------------------------------------------------
  
  // check valve status and timer to turn Valve Relay 3 ON
  if (valve_3_state == 0)
  {
    unsigned long waitTimeValveOff = 1000; // wait time (ms) from pump deactivation to relay deactivation
    if (myRTC.hours == valveRelay3_OnHour && myRTC.minutes == valveRelay3_OnMin && myRTC.seconds < ((waitTimePumpOn + waitTimeValveOff + MinTimePumpOperation) / 1000))
    {
    Serial.print("Valve Relay 3 Activation sequence RTC time: ");
    Serial.print(myRTC.hours);  // display the current hour from RTC module                
    Serial.print(":");                                                                                      
    Serial.print(myRTC.minutes);  // display the current minutes from RTC module            
    //Serial.print(":");                                                                                           
    //Serial.println(myRTC.seconds);  // display the seconds from RTC module 
    digitalWrite(pinValveRelay3, HIGH);
    valve_3_state = 1;
    Serial.println("*** Valve Relay 3 turned ON ***"); 
    // wait then turn pump relay ON
    Serial.print("Waiting ");
    Serial.print(waitTimePumpOn / 1000);
    Serial.println("s before activating Pump Relay.");
    delay(waitTimePumpOn);
    digitalWrite(pinPumpRelay, HIGH);
    pump_state = 1;
    Serial.println("*** Pump Relay turned ON ***");
    }
  }
  
  // check valve status and timer to turn Valve Relay 3 OFF
  if (valve_3_state == 1)
  {
    if (myRTC.hours == valveRelay3_OffHour && myRTC.minutes == valveRelay3_OffMin)
    {
    Serial.print("Valve Relay 3 Deactivation sequence RTC time: ");
    Serial.print(myRTC.hours);  // display the current hour from RTC module                
    Serial.print(":");                                                                                      
    Serial.print(myRTC.minutes);  // display the current minutes from RTC module            
    //Serial.print(":");                                                                                           
    //Serial.println(myRTC.seconds);  // display the seconds from RTC module
    digitalWrite(pinPumpRelay, LOW);
    pump_state = 0;
    Serial.println("*** Pump Relay turned OFF ***");
    valve_3_state = turnOffRelay (pinValveRelay3, 3);
    }
  }
  
  //------------------------------------------------------------------------------------
  // Current threshold monitor
  //------------------------------------------------------------------------------------
  
  if (millis() > 10000) // wait 10 secs after startup to avoid erroneous analog sensing
  {
    //Serial.println(millis());
    
  //------------------------------------------------------------------------------------
  // Print RTC time and sense current at regular interval 
  //------------------------------------------------------------------------------------                                   
   
  if (millis() - RTCtimeNow >= RTCtimeInterval)  // non-blocking. prints RTC time every time interval.
  {
    RTCtimeNow = millis();
    printMyTime ();
    AC_current = getIRMS();                                                          
  }

  
  // check if low current threshold
  if (AC_current <= LowCurrentLimit)
  {
    //LowCurrentLimit_state = 1;
    //count++; // increment current limit count by 1
    //Serial.print("*** Low current detected! ");
    //Serial.print(count);
    //Serial.println(" out of 3 ***");
    
    //if low current limit detected 3 counts in a row
    if (count == 3) // if current threshold low for about 10s, turn off pump relay
    {
      digitalWrite(pinPumpRelay, LOW); // turn pump off
      Serial.println("*** Low current limit! Pump Relay turned OFF ***");
      pump_state = 0; // stop monitoring current level
      count = 0; // reset current limit count
      
      // turning off valve relay
      if (valve_1_state == 1){valve_1_state = turnOffRelay (pinValveRelay1, 1);}
      if (valve_2_state == 1){valve_2_state = turnOffRelay (pinValveRelay2, 2);}
      if (valve_3_state == 1){valve_3_state = turnOffRelay (pinValveRelay3, 3);}
    }
  }

  // check high current threshold
  if (AC_current >= HighCurrentLimit)
  {
    //HighCurrentLimit_state = 1;
    //count++; // increment current limit count by 1
    //Serial.print("*** High current detected! ");
    //Serial.print(count);
    //Serial.println(" out of 3 ***");
    
    //if current limit detected 3 counts in a row
    if (count == 3) // if current threshold exceeded for about 10s, turn off pump relay
    {
      digitalWrite(pinPumpRelay, LOW); // turn pump off
      Serial.println("*** High current limit! Pump Relay turned OFF ***");
      pump_state = 0; // stop monitoring current level
      count = 0; // reset current limit count
    
      // turning off valve relay
      if (valve_1_state == 1){valve_1_state = turnOffRelay (pinValveRelay1, 1);}
      if (valve_2_state == 1){valve_2_state = turnOffRelay (pinValveRelay2, 2);}
      if (valve_3_state == 1){valve_3_state = turnOffRelay (pinValveRelay3, 3);}
    }
  }

  // if AC current within acceptable limits, reset current limit count
  if (AC_current < HighCurrentLimit && AC_current > LowCurrentLimit)
  {
    if (count != 0 && LowCurrentLimit_state == 1)
    {
      Serial.println("*** Low current limit count reset ***");
      LowCurrentLimit_state = 0;
    }
    if (count != 0 && HighCurrentLimit_state == 1)
    {
      Serial.println("*** High current limit count reset ***");
      HighCurrentLimit_state = 0;
    }
    count = 0; // reset current limit count
 }
}
}


