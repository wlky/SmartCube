#include "MacControl.h"


#define PORT 3000

byte macIP[] = {192, 168, 178, 118};

char macResponse[512];

void MacControl::sendCommandToMac(char requestBody[], char endpoint[]) {
    macHTTPClient->makeRequest(0, endpoint, macIP, PORT, true, "", "", "", requestBody, macResponse, 512, true, false);
}

void MacControl::toggleMute() {
    char endpoint[] = "/api/mute";
    sendCommandToMac("", endpoint);
}

void MacControl::increaseVolume() {
    char endpoint[] = "/controls/volume/80";
    sendCommandToMac("", endpoint);
}

void MacControl::decreaseVolume() {
    char endpoint[] = "/api/setvolume/30";
    sendCommandToMac("", endpoint);
}
extern char* itoa(int a, char* buffer, unsigned char radix);
void MacControl::setVolume(int volume) {
    char endpoint[256];
    char brightnessString[10];
    itoa(volume, brightnessString, 10);
    
    strcpy(endpoint, "/api/setvolume/");
    strcat(endpoint, brightnessString);
    sendCommandToMac("", endpoint);
}

void MacControl::readResponse() {
    if(macHTTPClient->isConnected()){
        macHTTPClient->readResponse();
    }
}
