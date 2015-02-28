#ifndef __HTTPCLIENT_H
#define __HTTPCLIENT_H

#include "application.h"
#include "spark_wiring_tcpclient.h"


/*----------------------------------------------------------------------*/
/* Macros and constants */
/*----------------------------------------------------------------------*/

#define REQUEST_LEN 800

class HTTPClient {
    /* Ethernet control */
    TCPClient myClient;


    unsigned long lastRead = millis();
    unsigned long firstRead = millis();
    bool error = false;
    bool timeout = false;

    bool connectionActive = false;

    // response mainbuffer
    char mainbuffer[512];
    char smallbuffer[85];

    // Allow maximum 5s between data packets.
    static const uint16_t REQUEST_TIMEOUT = 600;
    int readbytes = 0;

public:
    HTTPClient();
    virtual int readResponse();
    bool isConnected();
    virtual int makeRequest(unsigned short type,
            const char* url,
            byte* host,
            unsigned short port,
            boolean keepAlive,
            const char* contentType,
            const char* userHeader1,
            const char* userHeader2,
            const char* content,
            char* response,
            unsigned short responseSize,
            bool storeResponseHeader);

private:
    int sendRequest(byte* host, unsigned short port, char* response, unsigned short responseSize, bool storeResponseHeader);

};


#endif /* __HTTPCLIENT_H */