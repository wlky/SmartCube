#include "ClapDetection.h"
#include "application.h"
#define MIC A0


const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

int timeBetweenClaps = 1000;
int lastClap = 0;
int claps = 0;




int ClapDetection::detectClaps() {
  unsigned long startMillis= millis();  // Start of sample window
  unsigned int peakToPeak = 0;   // peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = 2040;
  int returnValue = -1;

  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(MIC);
    if(sample < 1000){
        continue;
    }
    
    if (sample > signalMax) {
      signalMax = sample;  // save just the max levels
    }
    else if (sample < signalMin) {
      signalMin = (signalMin+sample)/2;  // save just the min levels
    }
  }
  //Serial.print("signalMax: ");
 /* Serial.print(signalMax);
  Serial.print("\t");
  Serial.println(signalMin);
  */
  peakToPeak = signalMax - signalMin; //amplitude

  if(peakToPeak > 2200 && millis() - lastClap > 200) {
    Serial.print("Clapped: ");

    
    returnValue = 1;
    
    
    Serial.println(signalMin);
    Serial.println(" ");
    Serial.println(signalMax);
    Serial.println(peakToPeak);

    //Indicate one clap

    claps += 1;

    if(millis() - lastClap < timeBetweenClaps && claps == 2) {
        
      
        
      returnValue =2;
      Serial.println("Double clap");
      
     
      claps = 0;

      //Trigger action
    }
    else {
      claps = 1; 
    }

    lastClap = millis();
  }else if(millis() - lastClap > timeBetweenClaps && claps == 1){
      claps = 0;
  }
  
  return returnValue;
}
