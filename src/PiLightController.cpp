/*

#include "PiLightController.h"
#include "http_client.h"

#define LIB_DOMAIN "yanniks-pi.feste-ip.net"
byte bridgeIP[] = {192, 168, 0, 8};
byte pcIP[] = {10, 0, 1, 5};
char bridgeIP_str[] = "192.168.0.8";
char bridgeUser[] = "newdeveloper";
HTTPClient client;
extern char* itoa(int a, char* buffer, unsigned char radix);
unsigned long timeSince;

String request = "/parameter.php?red=";
char resp[1024];
byte server[] = {192, 168, 178, 44};

char tab2[200];
char endpoint[100];

void sendNormalizedColor(unsigned int r, unsigned int g, unsigned int b) {
    request = "/parameter.php?red=";
    request = request + r + "&green=" + g + "&blue=" + b + "&time=0";
    char tab2[200];
    strncpy(tab2, request.c_str(), sizeof (tab2));
    tab2[sizeof (tab2) - 1] = 0;

    client.makeRequest(0, tab2, server, 80, true, "", "", "", "", resp, 1024, true);
}

void sendColor(unsigned int r, unsigned int g, unsigned int b) {
    request = "/parameter.php?red=";
    request = request + r / 255.0 + "&green=" + g / 255.0 + "&blue=" + b / 255.0 + "&time=0";

    strncpy(tab2, request.c_str(), sizeof (tab2));
    tab2[sizeof (tab2) - 1] = 0;

    client.makeRequest(0, tab2, server, 80, true, "", "", "", "", resp, 1024, true);

}

void sendCommand(String command) {
    request = "/parameter.php?cmd=";
    request = request + command;
    strncpy(tab2, request.c_str(), sizeof (tab2));
    tab2[sizeof (tab2) - 1] = 0;

    client.makeRequest(0, tab2, server, 80, true, "", "", "", "", resp, 1024, true);
}
 * 
 * */