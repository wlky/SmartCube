/* 
 * File:   HueLightsControl.h
 * Author: Yannik
 *
 * Created on 27. Januar 2015, 20:29
 */

#ifndef HUELIGHTSCONTROL_H
#define	HUELIGHTSCONTROL_H
#include "http_client.h"
#include "application.h"

class HueLightsControl {
    HTTPClient* client;
public:
    HueLightsControl(HTTPClient* inClient){
        client = inClient;
    }
    void switchLightsOn(boolean on);
    void setBrightness(float rotationValue);
    void sendHeartBeat();
    bool sendColor(unsigned int r, unsigned int g, unsigned int b);
    void wlanRead();
    void sendCommandToHueBridge2(char requestBody[], char endpoint[]);
    int setBridgeIP(String bridgeIPString);
    
    bool forceTurnOff = false;
    bool forceTurnOn = false;
    bool offNotified = false;
    bool turnedOff = false;
    bool turnedOffSent = false;
};

#endif	/* HUELIGHTSCONTROL_H */

