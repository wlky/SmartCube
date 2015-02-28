#include "HueLightsControl.h"

#include "WS2812BSPIController.h"
#include <stdlib.h>

#define PORT 3000
#define TIME_BETWEEN_REQUESTS 100 //max 10 per second for bridge
#define REQUEST_LEN 800




    byte bridgeIP[] = {192, 168, 178, 118};
   // byte pcIP[] = {10, 0, 1, 5};
    //char bridgeIP_str[] = "192.168.0.8";
    char bridgeUser[] = "newdeveloper";
    
    char response[1024];
    extern char* itoa(int a, char* buffer, unsigned char radix);
    extern char* dtostrf (double val, signed char width, unsigned char prec, char *sout);
    unsigned long timeSince;

    char tab2[200];
    char requestBody[150];
    char endpoint[100];



int success = 0;





void HueLightsControl::sendCommandToHueBridge2(char requestBody[], char endpoint[]) {
  client->makeRequest(2, endpoint, bridgeIP, PORT, true, "application/json","", "", requestBody, response, 1024, true);
}


void HueLightsControl::wlanRead() {
    if (client->isConnected()) {
        client->readResponse();
        SPARK_WLAN_Loop();
    }
}

int HueLightsControl::setBridgeIP(String bridgeIPString) {
    char str[200];
    Serial.print("Setting bridgeIP to: ");
    Serial.println(bridgeIPString);
    strncpy(str, bridgeIPString.c_str(), sizeof (str));
    str[sizeof (str) - 1] = 0;

    unsigned short a, b, c, d;
    sscanf(str, "%hu.%hu.%hu.%hu", &a, &b, &c, &d);
    bridgeIP[0] = a;
    bridgeIP[1] = b;
    bridgeIP[2] = c;
    bridgeIP[3] = d;

    Serial.print("Set bridgeIP to: ");
    Serial.println(bridgeIP[0]);
    return 0;
}

typedef struct {
    double r; // percent
    double g; // percent
    double b; // percent
} rgb;

typedef struct {
    double h; // angle in degrees
    double s; // percent
    double v; // percent
} hsv;

static hsv rgb2hsv(rgb in);
static rgb hsv2rgb(hsv in);

hsv rgb2hsv(rgb in) {
    hsv out;
    double min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min < in.b ? min : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max > in.b ? max : in.b;

    out.v = max; // v
    delta = max - min;
    if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max); // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, v is undefined
        out.s = 0.0;
        out.h = 0; // its now undefined
        return out;
    }
    if (in.r >= max) // > is bogus, just keeps compilor happy
        out.h = (in.g - in.b) / delta; // between yellow & magenta
    else
        if (in.g >= max)
        out.h = 2.0 + (in.b - in.r) / delta; // between cyan & yellow
    else
        out.h = 4.0 + (in.r - in.g) / delta; // between magenta & cyan

    out.h *= 60.0; // degrees

    if (out.h < 0.0)
        out.h += 360.0;

    return out;
}

rgb hsv2rgb(hsv in) {
    double hh, p, q, t, ff;
    long i;
    rgb out;

    if (in.s <= 0.0) { // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long) hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch (i) {
        case 0:
            out.r = in.v;
            out.g = t;
            out.b = p;
            break;
        case 1:
            out.r = q;
            out.g = in.v;
            out.b = p;
            break;
        case 2:
            out.r = p;
            out.g = in.v;
            out.b = t;
            break;

        case 3:
            out.r = p;
            out.g = q;
            out.b = in.v;
            break;
        case 4:
            out.r = t;
            out.g = p;
            out.b = in.v;
            break;
        case 5:
        default:
            out.r = in.v;
            out.g = p;
            out.b = q;
            break;
    }
    return out;
}




void HueLightsControl::switchLightsOn(boolean on) {
    char trueOrFalse[20];
    if(on) {
      sprintf(trueOrFalse, "true");
    }
    else {
      sprintf(trueOrFalse, "false");
    }

    strcpy(requestBody, "{\"on\":");
    strcat(requestBody, trueOrFalse);
    strcat(requestBody, "}");

    char endpoint[200];
    // sprintf(endpoint, "/api/");
    // strcat(endpoint, bridgeUser);
    // strcat(endpoint, "/lights/1/state");

    sprintf(endpoint, "/api/");
    // strcat(endpoint, bridgeUser);
    // strcat(endpoint, "/groups/0/action");
    strcat(endpoint, "setBrightness");
	
    sendCommandToHueBridge2(requestBody, endpoint);
}

/*
	Send the color to the hue bridge
*/

bool HueLightsControl::sendColor(unsigned int r, unsigned int g, unsigned int b) {

	// convert the incoming RGB values into the HSV values the bridge needs
	rgb color;
	hsv HSVColor;
	color.r = r / 2.55;
	color.g = g / 2.55;
	color.b = b / 2.55;
	HSVColor = rgb2hsv(color);
	
	
	char brightnessString[10];
	itoa(int(ws2812BSPIController::dimmValue * 255), brightnessString, 10); // convert the dimm value into a string and save it in brightnessString
	strcpy(requestBody, "");
	sprintf(requestBody, "%s", "{\"bri\" : ");
	strcat(requestBody, brightnessString);

	itoa(int(HSVColor.h * 182), brightnessString, 10); // convert the hue value into a string and save it in brightnessString
	strcat(requestBody, ", \"hue\" : ");
	strcat(requestBody, brightnessString);

	itoa(int(HSVColor.s * 255), brightnessString, 10); // convert the saturation value into a string and save it in brightnessString
	strcat(requestBody, ", \"sat\" : ");
	strcat(requestBody, brightnessString); 
	
//  strcat(requestBody, ", \"transitiontime\" : 0");
	strcat(requestBody, "}");
	//now the request body should look like: {"bri" : ###, "hue" : ###, "sat" : ###}

	
	// set the endpoint to /api/setBrightness
	strcpy(endpoint, "");
	sprintf(endpoint, "/api/");
//  strcat(endpoint, bridgeUser);
//  strcat(endpoint, "/groups/0/action");
	strcat(endpoint, "setBrightness");
	
	

	// send the "turn lights on" command when the dimm value is below a certain value and the leds have not been turned off yet
	if ((ws2812BSPIController::dimmValue < 0.03 && !turnedOffSent) || forceTurnOff) {
		strcpy(requestBody, "{ \"on\" : false}");
		
		turnedOffSent = true;
		forceTurnOff = false;
	}
	
	// send the "turn lights off" command when the dimm value is above a certain value and the leds have turned off
	if ((ws2812BSPIController::dimmValue >= 0.03 && turnedOffSent) || forceTurnOn) {
		strcpy(requestBody, "{ \"on\" : true}");
		turnedOffSent = false;
		forceTurnOn = false;
	}

	// send the http request using the http client
	sendCommandToHueBridge2(requestBody, endpoint);
	return true;
}