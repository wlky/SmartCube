/* 
 * File:   MacControl.h
 * Author: Yannik
 *
 * Created on 28. Januar 2015, 14:20
 */

#ifndef MACCONTROL_H
#define	MACCONTROL_H


#include "application.h"
#include "http_client.h"
class MacControl {
    HTTPClient *macHTTPClient;
public:

    MacControl(HTTPClient *client) {
        macHTTPClient = client;
    }
    void toggleMute();
    void increaseVolume();
    void decreaseVolume();
    void setVolume(int volume);
    void readResponse();
private:
    void sendCommandToMac(char requestBody[], char endpoint[]);
};

#endif	/* MACCONTROL_H */

