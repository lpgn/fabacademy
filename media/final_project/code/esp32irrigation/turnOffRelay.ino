void turnOffRelay(int valveHere){
  // wait then turn valve relay OFF
  
  
  Serial.print("Waiting ");
  Serial.print(waitTimeValveOff / 1000);
  Serial.println("s before deactivating Valve Relay 3.");
  delay(waitTimeValveOff);
  digitalWrite(valveHere, LOW);
  valveHere = 0;
  Serial.print("*** Valve Relay ");
  Serial.print(valveHere);
  Serial.println("3 turned OFF ***");
}