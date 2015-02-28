#include "http_client.h"
//
// Receive HTTP Response
//
// The first value of client.available() might not represent the
// whole response, so after the first chunk of data is received instead
// of terminating the connection there is a delay and another attempt
// to read data.
// The loop exits when the connection is closed, or if there is a
// timeout or an error.

HTTPClient::HTTPClient() {
}


/*----------------------------------------------------------------------*/
/* make request - sends http request to a given host                    */

/* in the end the whole request is in the mainbuffer
   the smallbuffer is used to add the header files in the first part
   finally sendRequest(..) is called to actually execute the request
   
   PARAMETERS:
    type: 0 = GET, 1 = POST, 2 = PUT
    url: Path to the request
    host: bytefield with length of 4
          containing the bytes of the hosts IP
    keepAlive: boolean, send keep-alive otherwise send close
    userHeader1, userHeader2 additional headers, use empty string to skip
    content: the requests content
    response: buffer to store the response
    responseSize: size of the response buffer
    storeResponseHeader: wheter while reading it shall be tried
                         to only receive the body, detect for '\r\n\r\n'.
                         might not work in all cases, but might 
                         helt to reduce memory usage.

    HISTORY:
      12/7/14 PHHE
          Created the method.
      12/15/14 PHHE
          Added option to ignore the header

 */


int HTTPClient::makeRequest(unsigned short type,
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
        bool storeResponseHeader) {
//if(myClient.connected())return -1;
    // clear both buffers
    //TODO there was a & in front of smallbuffer
    memset(&smallbuffer, 0, sizeof (smallbuffer));
    memset(&mainbuffer, 0, sizeof (mainbuffer));
    memset(response, 0, responseSize);



    // add the first line of the header
    // scheme: [POST|GET] /url/... HTTP/1.1\r\n
    if (type == 0) {
        sprintf(mainbuffer, "GET ");
    } else if (type == 1) {
        sprintf(mainbuffer, "POST ");
    } else if (type == 2) {
        sprintf(mainbuffer, "PUT ");
    } else {
        // undefined request type
        // return
        return 1;
    }
    strcat(mainbuffer, url);
    strcat(mainbuffer, " HTTP/1.1\r\n");

    // add connection field, either keep-alive or close
    if (keepAlive) {
        strcat(mainbuffer, "CONNECTION: keep-alive\r\n");
    } else {
        strcat(mainbuffer, "CONNECTION: close\r\n");
    }

    // add the host IP and port to the header
    snprintf(smallbuffer, 85, "HOST: %d.%d.%d.%d:%d\r\n", host[0], host[1], host[2], host[3], port);
    strcat(mainbuffer, smallbuffer);
    memset(&smallbuffer, 0, sizeof (smallbuffer));

    // add content length to the header
    snprintf(smallbuffer, 85, "CONTENT-LENGTH: %d\r\n", strlen(content));
    strcat(mainbuffer, smallbuffer);
    memset(&smallbuffer, 0, sizeof (smallbuffer));

    // add the content type field
    snprintf(smallbuffer, 85, "CONTENT-TYPE: %s\r\n", contentType);
    strcat(mainbuffer, smallbuffer);
    memset(&smallbuffer, 0, sizeof (smallbuffer));

    // optionally add the userHeader1  
    if (strlen(userHeader1) > 0) {
        strcat(mainbuffer, userHeader1);
        strcat(mainbuffer, "\r\n");
    }

    // optionally add the userHeader2
    if (strlen(userHeader2) > 0) {
        strcat(mainbuffer, userHeader2);
        strcat(mainbuffer, "\r\n");
    }

    // end of header
    // one additional line break
    strcat(mainbuffer, "\r\n");

    // add the content
    strcat(mainbuffer, content);

    // actually send the stored request
    // return the return value of the send method
    return sendRequest(host, port, response, responseSize, storeResponseHeader);


}

int HTTPClient::readResponse() {
    if(myClient.connected()){
        myClient.flush();
    }else{
        myClient.stop();
        Serial.println("Connection stopped");
        connectionActive = false;
    }
}

bool HTTPClient::isConnected(){
    return connectionActive;
}


int HTTPClient::sendRequest (byte* host, unsigned short port, char* response, unsigned short responseSize, bool storeResponseHeader) 
{
    
  // this is the position in the response buffer
  short responsePosition=0;
  if (connectionActive || myClient.connect(host, port,300)) {
      connectionActive = true;
      uint32_t startTime = millis();
      
      myClient.write((const uint8_t *)mainbuffer, strlen(mainbuffer));
      myClient.flush();
      
      
      // wait for the other side to become available
      while(!myClient.available() && (millis() - startTime) < REQUEST_TIMEOUT){
          // not sure if it is a good thing to let our
          // local tcp activity be interrupted by cloud stuff...
         //delay(1);
         SPARK_WLAN_Loop();
      };

      // previous_c is used to recognize blanc line
      // intention is to seperate header from response body
      char previous_c = '\0';
      
      // current_c is where we store the current character
      // read from the TCPClients buffer
      char current_c = '\0';
      
      // responsePosition might differ from the 
      // currentPosition read from the TCPClients buffer
      // also it it will not be increased unless we reach 
      // the body, therefore we use currentPosition to store
      // the overall position read from the TCPClient
      short currentPosition=0;
      
      // flag used to seperate header from body
      // gets initiated with storeResponseHeader
      // so we assume to be already in the Body 
      // in case we shall return the whole response
      bool inBody = storeResponseHeader;
      
      
      // read the number of available bytes 
      int bytes = myClient.available();
      
      
      // loop as long as there is data
      while (bytes > 0 && !error) {
          // get number of available bytes
          
          // todo catch overflow here, after i<bytes
          for (int i = 0; i < bytes && !error; i++) {
            current_c =  myClient.read();
            
            // HTTP Status Code is expected to start at position 10
            // Only "200 OK" is defined in the range from 200-299,
            // so if we find a 2 at this position we can assume, that
            // we got a "200 OK", little dity but better than nothing
            // and fastv ;)
            if(!inBody && currentPosition==9 && current_c != '2') {
              
              #ifdef SERIAL_DEBUG
              Serial.println("HTTP Error Code != 200");
              Serial.println(current_c);
              #endif /* SERIAL_DEBUG */
              
              error = TRUE;
              break;
            }
            
            //detect the empty line between header and response
            if (!inBody && current_c == '\r' && previous_c == '\n') {
              inBody = TRUE;
            }
            
            if (inBody) {
              response[responsePosition++] = current_c;
            }
            
            currentPosition++;
            previous_c = current_c;
          }
          
          // this alternative for reading seems not work
          // maybe a future firmware version will fix this...
          // might be faster, but never the less with the problem that splitting 
          // header from body and parsing for error codes right here will no longer be possible 
          // readbytes = myClient.read((uint8_t*) &response[responsePosition], bytes);
          // responsePosition = responsePosition + bytes;
          
          bytes = myClient.available();
      }
      
      Serial.print("Received: ");
      Serial.println(responsePosition);
      
      //myClient.flush();
      //myClient.stop();
      
      if (error) {
        return -1;
      }

  } else {
      // unable to connect
      return -1; 
  }
  return responsePosition;
}