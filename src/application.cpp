// This #include statement was automatically added by the Spark IDE.
//#include "http_client.h"
#include "captouch.h"
#include "WS2812BSPIController.h"
#include "ClapDetection.h"
#include "HueLightsControl.h"
// This #include statement was automatically added by the Spark IDE.
//#include "captouch.h"

#include "SmartCube.h"
#include "MacControl.h"
#include <string.h>



SYSTEM_MODE(SEMI_AUTOMATIC);
/*############################ Gyro Declaration#############################*/


MPU6050 accelgyro;
ws2812BSPIController ledControl;

ClapDetection clapDetector;
HTTPClient client;
MacControl macControl(&client);
HueLightsControl hueControl(&client);

int16_t ax, ay, az;
int16_t gx, gy, gz;


#define OUTPUT_READABLE_ACCELGYRO

#define LED_PIN D7

CapTouch Touch(D4, D5);



float rotationSensitivity = 0.08;

bool blinkState = false;
int oldOrientation = 0;
unsigned long lastShow = millis();

unsigned long millisColorWasSent;
float dimmBuffer = 0;
float dimmValues[12] = {1,1,1,1,1,1,1,1,1,1,1,1};

unsigned long timeOfLastUserAction = 0;
bool sleeping = false;

int setBridgeIP(String ip){
    return hueControl.setBridgeIP(ip);
}

void setup() {

    Serial.begin(9600);

    Spark.variable("orientation", &oldOrientation, INT);
    Spark.function("notification", notification);
    Spark.function("setBridgeIP", setBridgeIP);
    //Spark.variable("lastTouchSapmle", &Touch.tDelay, INT);
    Spark.variable("touchdelay", &Touch.m_tReading, INT);
    Spark.variable("tochbaseline", &Touch.m_tBaseline, INT);
    
    Spark.variable("ax", &ax, INT);
    Spark.variable("ay", &ay, INT);
    Spark.variable("az", &az, INT);
    Spark.connect();
    pinMode(LED_PIN, OUTPUT);
    pinMode(D6, OUTPUT);


    //turn MPU6050 off
    digitalWrite(D6, LOW);
    pinMode(D0, INPUT);
    pinMode(D1, INPUT);
    delay(1000);
    //turn MPU6050 on
    digitalWrite(D6, HIGH);
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();
    // initialize serial communication

    delay(8000);
    Touch.setup();
    //Touch.setup();
    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();


    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // use the code below to change accel/gyro offset values

    // Serial.println("Updating internal sensor offsets...");
    //
    // Serial.print(accelgyro.getXAccelOffset());
    // Serial.print("\t");
    // Serial.print(accelgyro.getYAccelOffset());
    // Serial.print("\t");
    // Serial.print(accelgyro.getZAccelOffset());
    // Serial.print("\t");
    // Serial.print(accelgyro.getXGyroOffset());
    // Serial.print("\t");
    // Serial.print(accelgyro.getYGyroOffset());
    // Serial.print("\t");
    // Serial.print(accelgyro.getZGyroOffset());
    // Serial.print("\t");
    // Serial.print("\n");

    accelgyro.setXGyroOffset(110); // 90 = -80     180=280  180=290
    accelgyro.setYGyroOffset(-25); // 32 = 200    -120=-390  -60=-140
    accelgyro.setZGyroOffset(11); // -32 = -180  -180=-750    0=-50
    accelgyro.setXAccelOffset(-700); // 200 = -8000   300 = -7200    900 = -1600   940=-1100 1040=-300
    accelgyro.setYAccelOffset(-3355); // 900 = 22000   -3900 = -3500   -3300 = 1800  -3400=1000  -3600=-700     -3315
    accelgyro.setZAccelOffset(1250); // -400 = -14000   400 = -8000   900 = -3900   1200=-1400  1500=1000

    timeOfLastUserAction = millis();
    sleeping = false;
    oldOrientation = 0;
    pinMode(D2, OUTPUT);
    digitalWrite(D2, LOW);
    ledControl.ledSpiInit();

}

void pulse(int r, int g, int b, int millies, int ledNr) {
    int _delay = 20;
    float iterations = millies / _delay;
    int startR = ledControl.getR(ledNr);
    int startG = ledControl.getG(ledNr);
    int startB = ledControl.getB(ledNr);
    for (float j = 0; j < iterations; j++) {


        ledControl.setColor(ledNr, startR + ((r - startR) * j / (iterations)), startG + ((g - startG) * j / (iterations)), startB + ((b - startB) * j / (iterations)));
        ledControl.show();

        delay(_delay);

    }
    ledControl.setColor(ledNr, r, g, b);
}

int millisSleepDelay = 20000;

void resetSleepTimer() {
    timeOfLastUserAction = millis();
    sleeping = false;
}

bool isTimeToSleep() {
    return millis() > timeOfLastUserAction + millisSleepDelay;
}

void vibrate(int millis) {
    digitalWrite(D2, HIGH);
    delay(millis);
    digitalWrite(D2, LOW);
}


bool showingModes = false;
bool isTouching = false;

//Docking Station relevant stuff
uint8_t isInDockingStation = false;
long lastLoopTime = millis();
float currentColor[6][3] = {
    {0, 0, 0,}, /*  initializers for row indexed by 0 */
    {0, 0, 0}, /*  initializers for row indexed by 1 */
    {0, 0, 0}, /*  initializers for row indexed by 2 */
    {0, 0, 0}, /*  initializers for row indexed by 0 */
    {0, 0, 0}, /*  initializers for row indexed by 1 */
    {0, 0, 0}
};
unsigned short targetColor[6][3] = {
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4}

};
float difference[6][3] = {
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4},
    {4, 4, 4}
};

void initDockingStationMode() {
    oldOrientation = 10;
    for (int i = 0; i < 6; i++) {
        currentColor[i][0] = rand() % 255;
        currentColor[i][1] = rand() % 255;
        currentColor[i][2] = rand() % 255;
        targetColor[i][0] = rand() % 255;
        targetColor[i][1] = rand() % 255;
        targetColor[i][2] = rand() % 255;
        difference[i][0] = (rand() % 2) -1;
        difference[i][1] = (rand() % 2) -1;
        difference[i][2] = (rand() % 2) -1;
    }
    isInDockingStation = true;
}

void nextFadeStep(int timePassed) {
    for (int i = 0; i < 6; i++) {

        //        for (int j = 0; i < 3; j++) {
        //            currentColor[i][j] = currentColor[i][j] + difference[i][j] / 10;
        //
        //
        //
        //            if (
        //                    (difference[i][j] >= 0 && currentColor[i][j] >= targetColor[i][j])
        //                    || (difference[i][j] <= 0 && currentColor[i][j] <= targetColor[i][j])) {
        //                targetColor[i][0] = rand() % 255;
        //                targetColor[i][1] = rand() % 255;
        //                targetColor[i][2] = rand() % 255;
        //
        //                difference[i][0] = targetColor[i][0] - currentColor[i][0];
        //                difference[i][1] = targetColor[i][1] - currentColor[i][1];
        //                difference[i][2] = targetColor[i][2] - currentColor[i][2];
        //                j = 100;
        //            }
        //        }
        //Serial.println(currentColor[i][0]);
        //        ledControl.setColor(i, currentColor[i][0], currentColor[i][1], currentColor[i][2]);


        currentColor[i][0] = currentColor[i][0] + (difference[i][0] * timePassed * ((rand() % 100) / 500.0));
        currentColor[i][1] = currentColor[i][1] + (difference[i][1] * timePassed * ((rand() % 100) / 500.0));
        currentColor[i][2] = currentColor[i][2] + (difference[i][2] * timePassed * ((rand() % 100) / 500.0));
        for (int k = 0; k < 3; k++) {
            if (currentColor[i][k] > 255) {
                currentColor[i][k] = 255;
                difference[i][k] = -difference[i][k];
            }else if((difference[i][k] > 0) && (currentColor[i][k] > (190+rand()%65))){
                difference[i][k] = -difference[i][k];
            }
            if (currentColor[i][k] < 0) {
                currentColor[i][k] = 0;
                difference[i][k] = -difference[i][k];
            }
        }
        ledControl.setColor(i, currentColor[i][0], currentColor[i][1], currentColor[i][2]);
        
        

    }
    
            Serial.print("next step ");
    Serial.print(currentColor[0][0]);
    Serial.print("\t");
    Serial.print(currentColor[0][1]);
    Serial.print("\t");
    Serial.println(currentColor[0][2]);
    
}




unsigned long wantsToShowModes = 0;
unsigned long changeColorTime = millis();

void wlanRead() {

    hueControl.wlanRead();
    macControl.readResponse();
}

void loop() {
    //      if(!Serial.available()) {
    //        return;
    //      }
   Serial.print("a/g:\t");
   Serial.print(ax); Serial.print("\t");
   Serial.print(ay); Serial.print("\t");
   Serial.println(az); 
    wlanRead();

    if (millis() - lastShow > 30) {
        ledControl.show();
        lastShow = millis();
    }
    if (sleeping) {
        Serial.println("checking touchEvents ");
        checkTouchEvents();
        //sendNormalizedColor(Touch.test1, Touch.test2, 0);
        if (!isTouching) {
            delay(30);

            blinkState = !blinkState;
            digitalWrite(LED_PIN, blinkState);
            return;
        } else {
            transitionToMode(oldOrientation, 500);
            resetSleepTimer();
            isTouching = false;

        }
    }
    if (!accelgyro.testConnection()) {
        delay(50);
        if (!accelgyro.testConnection()) {
            for (int i = 0; i < 3; i++) {
                ledControl.setAllToColor(30, 0, 0);
                delay(200);
                ledControl.setAllToColor(0, 0, 0);
                delay(200);
            }
            delay(5000);
            if (!accelgyro.testConnection()) {
                delay(10000);
                if (!accelgyro.testConnection()) {
                    System.reset();
                }
            }
        }
    }

    if (isInDockingStation) {
        int timePassed = millis() - lastLoopTime;
        nextFadeStep(timePassed);
        lastLoopTime = millis();
    }



    wlanRead();

    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    int orientation = getOrientation(ax, ay, az);
    bool isInMotion;
    bool isCubeMoving = isMoving();



    if (showingModes && isCubeMoving) {
        isInMotion = true;
        resetSleepTimer();
    } else {
        isInMotion = inMotion();
    }

    wlanRead();


    if (orientation != oldOrientation && !isInMotion) {
        if (oldOrientation == -3 && orientation == 3) {
            if (!Spark.connected()) {
                Spark.connect();
            }
        }
        
        dimmValues[oldOrientation+3] = ws2812BSPIController::dimmValue;
        ws2812BSPIController::dimmValue =  dimmValues[orientation+3];
        isInDockingStation = false;
        oldOrientation = orientation;
        //ws2812BSPIController::dimmValue = 1;
        switchMode(oldOrientation);
        resetSleepTimer();
        Serial.println("new orientation " + orientation);
        vibrate(250);
    }

    if (!isCubeMoving) {
        int claps = 0;
        //claps = clapDetector.detectClaps();

        switch (claps) {
            case 1:
//                ledControl.setAllToColor(255, 0, 255);
//                delay(100);
//                switchModeLocal(oldOrientation);
                break;
            case 2:
//                ledControl.setAllToColor(0, 255, 200);
//                delay(100);
//                switchModeLocal(oldOrientation);
                sendCommand("toggle");
                break;
        }
    }

    wlanRead();

    if (isInMotion) {
        if (!wantsToShowModes) {
            wantsToShowModes = millis();
        }
    } else {
        wantsToShowModes = 0;
        if (showingModes) {
            switchMode(oldOrientation);
        }
    }

    if (!showingModes && wantsToShowModes != 0 && millis() - wantsToShowModes > 150) {
        showModes();
    }


    if (isTimeToSleep() && !sleeping && !isInDockingStation) {
        ledControl.transitionAllToColor(0, 0, 0, 500);
        Touch.reset();
        //WiFi.off();
        sleeping = true;
    }

    if (!getIsPulsing() && !isInMotion) {
        float rotVal = float(getRotationValue(orientation, gx, gy, gz)) / 32768; //Normalisierte Werte 16 bit
        if (rotVal > 0.005 || rotVal < -0.005) {
            dimmBuffer += rotVal;
        }
        if (dimmBuffer > 0.05 || dimmBuffer < -0.05) {
            setDimmValue(dimmBuffer);
            switchMode(oldOrientation);
            dimmBuffer = 0;
            Serial.println("cube is moving");
            changeColorTime = millis();

        }
        /*
         if(millis() - lastSetColorTime > 100 && !sleeping){
            
             sendColor(getR(0), getG(0), getB(0));
            
             lastSetColorTime = millis();
         }
         */



    }

    if (getIsPulsing()) {
        //pulse();
    }

    // these methods (and a few others) are also available
    //accelgyro.getAcceleration(&ax, &ay, &az);
    //accelgyro.getRotation(&gx, &gy, &gz);


    if (changeColorTime > millisColorWasSent) {
        Serial.print("changeColorTime: ");
        Serial.print(changeColorTime);
        Serial.print(" lastSetColorTime: ");
        Serial.print(millisColorWasSent);

        long newUpdateTime = updateColor();

        if (newUpdateTime != -1) {
            millisColorWasSent = newUpdateTime;
        }
    }
    if(!isInDockingStation){
        checkForDockingstation();
    }

    wlanRead();

    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
}

void checkTouchEvents() {
    CapTouch::Event touchEvent = Touch.getEvent();
    if (!isInDockingStation) {
        if (Touch.m_tReading > 2100) {

            initDockingStationMode();
            Serial.println(" Received Dock");
            return;
        }
    }
    if (touchEvent == CapTouch::TouchEvent) {
        isTouching = true;
        Serial.println(" Received Touch");
    } else if (touchEvent == CapTouch::ReleaseEvent) {
        isTouching = false;
        Serial.println(" Received Release");
    }
}
unsigned long millisLastDockingstationCheck = millis();

void checkForDockingstation() {
    if (millis() - millisLastDockingstationCheck > 1000) {
        CapTouch::Event touchEvent = Touch.getEvent();
        if (!isInDockingStation) {
            if (Touch.m_tReading > 1900) {

                initDockingStationMode();
                Serial.println(" Received Dock");
                return;
            }
        }
        millisLastDockingstationCheck = millis();
    }
}

unsigned long updateColor() {
    if (millis() - millisColorWasSent > 200 && !sleeping) {
        Serial.println("updating color");
        sendColor(ledControl.getR(0), ledControl.getG(0), ledControl.getB(0));
        return millis();
    }
    return -1;
}

unsigned long sendColor(unsigned int r, unsigned int g, unsigned int b) {
    if (oldOrientation == -3) {
        macControl.setVolume(int(ws2812BSPIController::dimmValue * 100));
        
        return millis();
    } else //if (!client2.isConnected()) 
    {
        hueControl.sendColor(r, g, b);
        
        
        return millis();

    }
    return -1;
}

void sendCommand(String command) {
    if (command == "off") {
        hueControl.forceTurnOff = true;
    }
    if (command == "on") {
        hueControl.forceTurnOn = true;
    }
    if (command == "toggle") {
        if (hueControl.turnedOff) {
            hueControl.forceTurnOn = true;
        } else {

            hueControl.forceTurnOff = true;
        }
    }
    Serial.print("Send command: ");
    Serial.println(command);
    changeColorTime = millis();
}

void showModes() {

    showingModes = true;
    ledControl.setColor(4, 255, 255, 255);
    ledControl.setColor(0, 255, 0, 255);
    ledControl.setColor(5, 255, 255, 0);
    ledControl.setColor(2, 0, 0, 255);
    ledControl.setColor(3, 0, 255, 0);
    ledControl.setColor(1, 255, 0, 0);
    ledControl.show();
}

void switchMode(int _orientation) {
    Serial.println("Switching mode");
    switchModeLocal(_orientation);
    updateColor();
}

void switchModeLocal(int _orientation) {
    showingModes = false;
    int dimmedValue = (int) (255 * ws2812BSPIController::dimmValue);
    switch (_orientation) {
        case -1:
            //Serial.println("-1");
            ledControl.setAllToColor(dimmedValue, dimmedValue, dimmedValue);
            break;
        case 1:
            //Serial.println("1");
            ledControl.setAllToColor(dimmedValue, 0, dimmedValue);
            break;
        case -2:
            //Serial.println("-2");
            ledControl.setAllToColor(dimmedValue, dimmedValue, 0);
            break;
        case 2:
            //Serial.println("2");
            ledControl.setAllToColor(0, 0, dimmedValue);
            break;
        case -3:
            //Serial.println("-3");
            ledControl.setAllToColor(0, dimmedValue, 0);
            break;
        case 3:
            //Serial.println("3");
            ledControl.setAllToColor(dimmedValue, 0, 0);
            break;
        default:
            //Serial.println("None");
            ledControl.setAllToColor(dimmedValue, dimmedValue, dimmedValue);

            break;
    }
}

void transitionToMode(int _orientation, int timeMillis) {
    showingModes = false;
    int dimmedValue = (int) (255 * ws2812BSPIController::dimmValue);
    switch (_orientation) {
        case -1:
            //Serial.println("-1");
            ledControl.transitionAllToColor(dimmedValue, dimmedValue, dimmedValue, timeMillis);
            break;
        case 1:
            //Serial.println("1");
            ledControl.transitionAllToColor(dimmedValue, 0, dimmedValue, timeMillis);
            break;
        case -2:
            //Serial.println("-2");
            ledControl.transitionAllToColor(dimmedValue, dimmedValue, 0, timeMillis);
            break;
        case 2:
            //Serial.println("2");
            ledControl.transitionAllToColor(0, 0, dimmedValue, timeMillis);
            break;
        case -3:
            //Serial.println("-3");
            ledControl.transitionAllToColor(0, dimmedValue, 0, timeMillis);
            break;
        case 3:
            //Serial.println("3");
            ledControl.transitionAllToColor(dimmedValue, 0, 0, timeMillis);
            break;
        default:
            //Serial.println("None");
            ledControl.transitionAllToColor(dimmedValue, dimmedValue, dimmedValue, timeMillis);

            break;
    }
    updateColor();
}

int getRotationValue(int _orientation, int _gx, int _gy, int _gz) {
    switch (_orientation) {
        case -1:
            return _gx;
        case 1:
            return _gx * (-1);
        case -2:
            return _gy;
        case 2:
            return _gy * (-1);
        case -3:
            return _gz;
        case 3:
            return _gz * (-1);

        default:
            return 0;
    }
}

int getOrientation(int _ax, int _ay, int _az) {
    int x = _ax;
    int y = _ay;
    int z = _az;
    int absX = abs(x);
    int absY = abs(y);
    int absZ = abs(z);

    if (absX > absY && absX > absZ) {
        if (x > 0) {
            return 1;
        }
        return -1;
    }
    if (absY > absX && absY > absZ) {
        if (y > 0) {
            return 2;
        }
        return -2;
    }
    if (absZ > absX && absZ > absY) {
        if (z > 0) {

            return 3;
        }
        return -3;
    }
    return 0;
}

int notification(String _type) {
    setIsPulsing(true);
    if (_type == "facebook") {
        ledControl.transitionAllToColor(0, 0, 255,500);
       
        
        Serial.println("facebook incoming");
    } else if (_type == "twitter") {
        ledControl.transitionAllToColor(0, 255, 255,500);
        Serial.println("twitter incoming");
    } else if (_type == "whatsapp") {
        ledControl.transitionAllToColor(0, 255, 0,500);
        Serial.println("whatsapp incoming");
    } else if (_type == "email") {
        ledControl.transitionAllToColor(255, 0, 0,500);
        Serial.println("email incoming");
    } else if (_type == "stop") {
        setIsPulsing(false);
        Serial.println("stopped pulsing");
    } else {

        ledControl.transitionAllToColor(255, 255, 255,500);
        Serial.println(_type);
    }
    
     ledControl.transitionAllToColor(0, 0, 0,500);
    switchMode(oldOrientation);
    return 0;
}


int r, g, b;

void setDimmValue(float _rotationValue) {
    
    if(oldOrientation == -3){
         rotationSensitivity = 0.05;
    }else{
        rotationSensitivity = 0.1;
    }
    
    if (_rotationValue > 0.01 || _rotationValue < -0.01) { //abs() doesnt work ?!

        ws2812BSPIController::dimmValue += (_rotationValue * rotationSensitivity);

        if (ws2812BSPIController::dimmValue < 0) {
            ws2812BSPIController::dimmValue = 0;
        } else if (ws2812BSPIController::dimmValue > 1) {
            ws2812BSPIController::dimmValue = 1;
        }

        if (ws2812BSPIController::dimmValue < 0.05) {
            if (hueControl.offNotified && !hueControl.turnedOff) {
                ws2812BSPIController::dimmValue = 0;
                hueControl.turnedOff = true;
                hueControl.offNotified = false;
            } else if (!hueControl.turnedOff) {
                vibrate(200);
                hueControl.offNotified = true;
            }
        } else if (hueControl.turnedOff) {

            vibrate(200);
            hueControl.turnedOff = false;
        }

        //  Serial.println(_rotationValue);
        resetSleepTimer();
    }
}

float getDimmValue() {

    return ws2812BSPIController::dimmValue;
}

bool isPulsing = false;

void setIsPulsing(bool _isPulsing) {

    isPulsing = _isPulsing;
}

bool getIsPulsing() {

    return isPulsing;
}

int value;
long currentTime = 0;
int periode = 2000;
float PI = 3.141F;

void pulse() {

    currentTime = millis();
    //  value = 128+127*cos(2*PI/periode*currentTime);
    /*
        analogWrite(R, r * value / 255);
        analogWrite(G, g * value / 255);
        analogWrite(B, b * value / 255);
     */
}

int gyroTollerance = 400;
int accTollerance = 1600;

bool isMoving() {
    int absGx = abs(gx);
    int absGy = abs(gy);
    int absGz = abs(gz);

    // if(showingModes){
    if ((absGx + absGy + absGz) <= gyroTollerance) {
        return false;
    } else {

        return true;
    }
    //}

}

bool inMotion() {
    //Cube is not moving - Check if it's laying completely on one side
    int absX = abs(ax);
    int absY = abs(ay);
    int absZ = abs(az);

    int max = getMax(absX, absY, absZ);

    //Test for usual acceleration by earth (adjust your values if needed)
    if (max < (16900 - accTollerance) || max > (16900 + accTollerance)) {
        return true;
    } else {
        //Cube is not moving!

        return false;
    }

}

int getMax(int a, int b, int c) {
    int max = a;

    if (b >= a && b >= c) {
        max = b;
    } else if (c >= a && c >= b) {
        max = c;
    }

    return max;
}
