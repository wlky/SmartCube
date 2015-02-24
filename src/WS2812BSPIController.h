/* 
 * File:   WS2812BSPIController.h
 * Author: Yannik
 *
 * Created on 27. Januar 2015, 20:11
 */

#ifndef WS2812BSPICONTROLLER_H
#define	WS2812BSPICONTROLLER_H
#include "application.h"
class ws2812BSPIController {
public:
    void setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue);
    uint8_t getR(uint16_t aLedNumber);
    uint8_t getG(uint16_t aLedNumber);
    uint8_t getB(uint16_t aLedNumber);
    void show();
    void ledSpiInit();
    void setAllToColor(int r, int g, int b);
    void transitionAllToColor(int r, int g, int b, int millies);
    
    
    static float dimmValue;

protected:

};




#endif	/* WS2812BSPICONTROLLER_H */

