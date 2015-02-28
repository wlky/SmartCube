// This #include statement was automatically added by the Spark IDE.
//#include "http_client.h"
#include "captouch.h"
#include "WS2812BSPIController.h"
#include "ClapDetection.h"
#include "HueLightsControl.h"
#include "SmartCube.h"
#include "MacControl.h"
#include <string.h>


//Manually connect to the wifi &cloud. This way the startup process is not blocked while waiting for a wifi connection
SYSTEM_MODE(SEMI_AUTOMATIC);


/*############################ Gyro Declaration#############################*/
MPU6050 accelgyro;
ws2812BSPIController ledControl;
int16_t ax, ay, az; //3-axis acceleration variables
int16_t gx, gy, gz; //3-axis rotation variables


/*############################ Controller Declaration#############################*/
HTTPClient client; //initialize a httpclient
MacControl macControl(&client); //Init the controller objects 
HueLightsControl hueControl(&client);


ClapDetection clapDetector;

#define LED_PIN D7  //define output LED
#define VIBRATION_MOTOR_PIN D2  //define vibration motor pin
#define TOUCH_OUTPUT D4
#define TOUCH_INPUT D5
#define MPU6050_POWER_PIN D6
#define SDA_PIN D0
#define SDX_PIN D1

int millisSleepDelay = 20000; // time to wait for user action before going to sleep
int gyroTollerance = 400; // tollerance of movement noise. Values greater than this will be detected as movements
int accTollerance = 1600; // amount the acceleration of one axis can deviate from the standard gravity value before being detected as no more laying flat on one side
float rotationSensitivity = 0.08; //define rotation sensitivity 

CapTouch Touch(TOUCH_OUTPUT, TOUCH_INPUT);//init capacitive touch detection





bool blinkState = false; //alternating state changing after each loop cycle, shown
int oldOrientation = 0; //the last valid orientation
unsigned long lastShow = millis(); //when was the last time the led color has been updated

unsigned long millisColorWasSent; // when was the last time the color was sent over the network
float dimmBuffer = 0;
float dimmValues[12] = {1,1,1,1,1,1,1,1,1,1,1,1}; // memory of the dimm values of each side of the cube
unsigned long wantsToShowModes = 0; // when did the gyro detect a motion that is within the threshhold of showing the modes
#define DELAY_BEFORE_SHOWING_MODES 150 //delay in ms to wait between wantsToShowModes and actually showing the modes. An increased delay filters out more gyro fluptuations but is less responsive to user input
unsigned long changeColorTime = millis(); // when was the last time the color was changed
unsigned long timeOfLastUserAction = 0; // when was the last time that some sort of input from the user was detected
bool sleeping = false; //is the device sleeping or active (sleeping = leds off, gyro off, waiting for touchevent)
bool showingModes = false; // is the cube currently showing the modes (e.g. showing which color is on which side)
bool isTouching = false; // is the cube being touched
// set the host ip address of the philipps hue bridge
// ip must be a String with a IPv4 address (e.g. 192.168.1.2)
int setBridgeIP(String ip){
    return hueControl.setBridgeIP(ip);
}

void setup() {
    Serial.begin(9600); // initialize serial communication
	
	/*############################# Spark Cloud init ###############################*/
    Spark.variable("orientation", &oldOrientation, INT); //
    Spark.function("notification", notification);
    Spark.function("setBridgeIP", setBridgeIP);
    //Spark.variable("lastTouchSapmle", &Touch.tDelay, INT); //for debug purposes: what was the last delay measured
    Spark.variable("touchdelay", &Touch.m_tReading, INT);
    Spark.variable("tochbaseline", &Touch.m_tBaseline, INT);

    Spark.connect(); // enable wifi and connect to spark cloud
    pinMode(LED_PIN, OUTPUT); //set the LED pin to Output
    pinMode(MPU6050_POWER_PIN, OUTPUT); // set the mpu6050 power controller pin to Output


    //turn MPU6050 off
    digitalWrite(MPU6050_POWER_PIN, LOW);
    pinMode(SDA_PIN, INPUT);
    pinMode(SDX_PIN, INPUT);
    delay(100);
	
    //turn MPU6050 on
    digitalWrite(MPU6050_POWER_PIN, HIGH);
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();
    

    delay(8000); //wait some time before starting loop to be able to start a firmware update in case starting the loop breaks the device
    Touch.setup(); //make initial capacitive touch baseline measurements
	
    // initialize MPU6050
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

    timeOfLastUserAction = millis(); //set time of last user action to current time
    sleeping = false; // set the state to not sleeping
    oldOrientation = 0; // set the initial orientation
    pinMode(VIBRATION_MOTOR_PIN, OUTPUT); //set the vibration motor pin to Output
    digitalWrite(VIBRATION_MOTOR_PIN, LOW); 
    ledControl.ledSpiInit(); // init the SPI connection of the WS2812BSPIController

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



void resetSleepTimer() {
    timeOfLastUserAction = millis();
    sleeping = false;
}

bool isTimeToSleep() {
    return millis() > timeOfLastUserAction + millisSleepDelay;
}

/*
	Vibrate for a certain amount of time
	TODO: make this method non-blocking (without using the delay function)
*/
void vibrate(int millis) {
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    delay(millis);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
}




//Docking Station LED fading relevant stuff
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
}
//tell the controllers to read the response from the network
void wlanRead() {
    hueControl.wlanRead();
    macControl.readResponse();
}

/*################################## MAIN LOOP #######################################*/
void loop() {
	// Update the color of the LEDs
    if (millis() - lastShow > 30) {
        ledControl.show();
        lastShow = millis();
    }
	
	/*
		When the cube is in the sleep mode only check for touch events
	*/
	
    if (sleeping) {
        Serial.println("checking touchEvents ");
        checkTouchEvents();
        if (!isTouching) { // when no touch is detected
            delay(30); // wait a bit and try again
			
            blinkState = !blinkState; //and blink the status led
            digitalWrite(LED_PIN, blinkState);
			
            return;
        } else { // when no touch is detected
            transitionToMode(oldOrientation, 500); //turn back on to the last state known
            resetSleepTimer(); // and reset the sleep timer
            isTouching = false;
        }
    }
	
	// Gyro test. Before reading from the gyro first check whether the connection is working. This test mostly fails when the battery is getting low.
    if (!accelgyro.testConnection()) { //if the connection is not working
        delay(50);					// wait a bit
        if (!accelgyro.testConnection()) { // and try again, if it is still not working
            for (int i = 0; i < 3; i++) {				//indicate by blinking the leds
                ledControl.setAllToColor(30, 0, 0);
                delay(200);
                ledControl.setAllToColor(0, 0, 0);
                delay(200);
            }
            delay(5000);			// wait for a while
            if (!accelgyro.testConnection()) { // and try again
                delay(10000);
                if (!accelgyro.testConnection()) {
                    System.reset();
                }
            }
        }
    }

	
	// Docking station test
    if (isInDockingStation) { // when the device is in docking station mode
        int timePassed = millis() - lastLoopTime; // calculate the time since last loop
        nextFadeStep(timePassed); // and let the fade method know how much time has passed.
        lastLoopTime = millis();
    }

    wlanRead(); // Read the html data before doing the gyro calculations

    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // get the current motion data from the MPU6050
    int orientation = getOrientation(ax, ay, az); // calculate the current orientation
	
    bool isInMotion; // this variable indicates whether the cube is in motion
    bool isCubeMoving = isMoving(); // this variable is always true, when the cube is moving in any direction

	
	/*
		Determine, whether the cube is moving
	*/
    if (showingModes && isCubeMoving) { // when the cube is currently showing the modes, and is moving
        isInMotion = true; // the cube is seen as moving
        resetSleepTimer();
    } else { // when the cube is not showing the modes or the cube is not moving
        isInMotion = inMotion(); // the cube is only seen as moving, when it is not laying flat on one side
    }

    wlanRead(); // Read the html data after doing the gyro calculations


	
    if (orientation != oldOrientation && !isInMotion) { // when the orientation of the cube has changed and the cube is not moving
        if (oldOrientation == -3 && orientation == 3) { // DEBUG way of re enabling the connection to wifi and cloud
            if (!Spark.connected()) {
                Spark.connect();
            }
        }
        
        dimmValues[oldOrientation+3] = ws2812BSPIController::dimmValue; // save the value to which the color on the previous orientation was dimmed 
        ws2812BSPIController::dimmValue =  dimmValues[orientation+3]; // retreive the dimm value for the new orientation
        isInDockingStation = false; // reset the docking station mode (in case it was true, and the cube is taken out of the docking station and layd down)
        oldOrientation = orientation; // save the new orientation
        switchMode(oldOrientation); // set the mode based on the new orientation
        resetSleepTimer();
        Serial.println("new orientation " + orientation);
        vibrate(250); // vibrate to give tactical feedback
    }

    if (!isCubeMoving) { // when the cube is not moving
        int claps = 0;
        claps = clapDetector.detectClaps(); //detect claps 

        switch (claps) { //based on the number of claps, do stuff
            case 1: // in case of one clap
//                ledControl.setAllToColor(255, 0, 255);
//                delay(100);
//                switchModeLocal(oldOrientation);
                break;
            case 2: // in case of two claps
//                ledControl.setAllToColor(0, 255, 200);
//                delay(100);
//                switchModeLocal(oldOrientation);
                sendCommand("toggle"); //toggle the light (on->off, off->on)
                break;
        }
    }

	
	/*
		debouncer for motion detection. Since the motion detection sometimes detects wrong peaks of movement for a very short amount of time,
		we remember the first time the movement was detected, wait for DELAY_BEFORE_SHOWING_MODES millis, check again whether the cube is still detected as moving.
		If so, the movement is seen as genuine and the cube changes its mode to showingmodes
	*/
    if (isInMotion) { // when cube is moving
        if (!wantsToShowModes) { // and no cube movement has previously been detected
            wantsToShowModes = millis(); // remember the first time a movement is detected
        }
    } else { // when the cube is not moving
        wantsToShowModes = 0; // reset the last time a cube movement was detected
        if (showingModes) { // and in case we are currently showing the modes
            switchMode(oldOrientation); // show the last remembered orientation
        }
    }

    if (!showingModes // when we are not already showing the modes
		&& wantsToShowModes != 0  // and a movement has been detected, and not been resetted in the meanwhile
		&& millis() - wantsToShowModes > DELAY_BEFORE_SHOWING_MODES) { // and the delay time has passed
        showModes(); // switch the mode to showingmodes
    }


	/*
		sleep timer
		transition to sleep mode when no user action has been detected for a while
	*/
    if (isTimeToSleep() && !sleeping && !isInDockingStation) {
        ledControl.transitionAllToColor(0, 0, 0, 500);
        Touch.reset();
        //WiFi.off();
        sleeping = true;
    }

	/*
		Gyro calculations to detect rotations around the horizontal axis and change the dimm value of the current mode accordingly
	*/
    if (!getIsPulsing() && !isInMotion) { // in case the cube is not moving (is laying flat on one side and not showing modes)
        float rotVal = float(getRotationValue(orientation, gx, gy, gz)) / 32768; //Calculate the movement around the vertical axis based on the current orientation
        if (rotVal > 0.005 || rotVal < -0.005) { // in case the detected rotation is greater than the normal movement "noise" 
            dimmBuffer += rotVal; // add the detected rotation to the previously detected rotations
        }
        if (dimmBuffer > 0.05 || dimmBuffer < -0.05) { // in case the sum of detected rotations is greater than a certain threshhold
            setDimmValue(dimmBuffer); // update the dimm value for the current mode based on the sum of rotations
            switchMode(oldOrientation);
            dimmBuffer = 0; // and reset the rotation sum
            changeColorTime = millis(); // also set the time at which the color was changed
        }
    }

	/*
		Update the network device if necessary
	*/
    if (changeColorTime > millisColorWasSent) { // check whether the color has changed since the last time the network device was updated
        long newUpdateTime = updateColor(); // if so, try to update the color

        if (newUpdateTime != -1) { // if the color was successfully updated, updateColor will have returned the new update time
            millisColorWasSent = newUpdateTime; // and the time of the last update of the network device gets set accordingly
        }
    }
	
	/*
		Docking station detection
	*/
    if(!isInDockingStation){
        checkForDockingstation();
    }

    wlanRead(); // Read http data at the end of the loop

    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
}

/*
	Check for changes in the capacity.
	Changes happen for example when the cube is touched, or connected via usb
*/
void checkTouchEvents() {
    CapTouch::Event touchEvent = Touch.getEvent(); // do the actual capacity test
    if (!isInDockingStation) { // in case the cube is not in the docking station mode
        if (Touch.m_tReading > 1900) { // and the result of the capacity test is greater than when its not connected over usb
            initDockingStationMode(); // initialize the docking station mode
            // Serial.println(" Received Dock");
			
            return; //dont check for other touch events
        }
    }
	
    if (touchEvent == CapTouch::TouchEvent) { // in case a touchevent is detected
        isTouching = true;	// set the variable accordingly
        Serial.println(" Received Touch");
    } else if (touchEvent == CapTouch::ReleaseEvent) { // in case the touch stops
        isTouching = false; // set the variable accordingly
        Serial.println(" Received Release");
    }
}
unsigned long millisLastDockingstationCheck = millis();

void checkForDockingstation() {
    if (millis() - millisLastDockingstationCheck > 1000) {
        CapTouch::Event touchEvent = Touch.getEvent();
		if (!isInDockingStation) { // in case the cube is not in the docking station mode
			if (Touch.m_tReading > 1900) { // and the result of the capacity test is greater than when its not connected over usb
				initDockingStationMode(); // initialize the docking station mode
			}
		}
        millisLastDockingstationCheck = millis();
    }
}

/*
	Update the data on the connected network device, restricted to a minimum time between the updates.
*/
unsigned long updateColor() {
    if (millis() - millisColorWasSent > 200 && !sleeping) {
        Serial.println("updating color");
        sendColor(ledControl.getR(0), ledControl.getG(0), ledControl.getB(0));
        return millis();
    }
    return -1;
}

/*
	Send the color data to the connected network device
*/
unsigned long sendColor(unsigned int r, unsigned int g, unsigned int b) {
    if (oldOrientation == -3) { //in case the cube is currently in the volume control mode
        macControl.setVolume(int(ws2812BSPIController::dimmValue * 100)); // actually dont send color information, but volume info
        return millis();
    } else //if (!client2.isConnected()) 
    {
        hueControl.sendColor(r, g, b);
        return millis();

    }
    return -1;
}

/*
	send commands to connected network devices like "light off"
*/
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

/*
	Indicates which mode is on which side.
	Indication is done by illuminating each side of the cube with a different color that represents what will happen when the cube is put down with the respective side facing upwards
*/
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

/*
	Switch to the mode of the parameter _orientation and inform the network device of the change
*/
void switchMode(int _orientation) {
    Serial.println("Switching mode");
    switchModeLocal(_orientation);
    updateColor();
}

/*
	Switch to the mode of the parameter _orientation
*/
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

/*
	Fade from the current color to the color specified by _orientation
	timeMillis: the time the fading takes in ms
*/
void transitionToMode(int _orientation, int timeMillis) {
    showingModes = false;
    int dimmedValue = (int) (255 * ws2812BSPIController::dimmValue); //convert the dimmvalue from a float between 0 and 1 to an integer between 0 and 255
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

/*
	Determines the correct rotation value based on the current orientation
*/
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

/*
	Determine the orientation of the cube based on the acceleration values
*/
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

/*
	Lets the leds pulse in a certain color based on the notification type
*/
int notification(String _type) {
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
    
    ledControl.transitionAllToColor(0, 0, 0,500); //fade the colors back to 0
    switchMode(oldOrientation); // show the mode that was showing before the notification
    return 0;
}


int r, g, b;

/*
	Update the dimm value of the current mode
*/
void setDimmValue(float _rotationValue) {
    
    if(oldOrientation == -3){ //when the cube is in sound mode
         rotationSensitivity = 0.05; // reduce the sensitivity
    }else{
        rotationSensitivity = 0.1; // else set the sensitivity to a higher value
    }
    
    if (_rotationValue > 0.01 || _rotationValue < -0.01) { //only change something if the incoming rotation value is greater than a certain threshhold

        ws2812BSPIController::dimmValue += (_rotationValue * rotationSensitivity); //modify the dimm value based on the rotation and the rotationsensitivity
		
		// Ensure minimum of 0 and maximum of 1
        if (ws2812BSPIController::dimmValue < 0) {
            ws2812BSPIController::dimmValue = 0;
        } else if (ws2812BSPIController::dimmValue > 1) {
            ws2812BSPIController::dimmValue = 1;
        }
		
		//Give feedback before turning off the leds
        if (ws2812BSPIController::dimmValue < 0.05) { // when the dimm value is low
            if (hueControl.offNotified && !hueControl.turnedOff) { // and the feedback has already been given, but the leds have not turned off yet
                ws2812BSPIController::dimmValue = 0; // turn the leds off
                hueControl.turnedOff = true; // reset the values for next time
                hueControl.offNotified = false;
            } else if (!hueControl.turnedOff) { // when the feedback has not been given and the leds have not been turned off
                vibrate(200); // give the feedback
                hueControl.offNotified = true; // remember that the feedback was given
            }
        } else if (hueControl.turnedOff) { // when the leds are turned off and dimm value is passing the lower threshhold
            vibrate(200); // give feedback that the leds are turned on
            hueControl.turnedOff = false; // remeber that the leds are turned on
        }

        //  Serial.println(_rotationValue);
        resetSleepTimer();
    }
}

float getDimmValue() {
    return ws2812BSPIController::dimmValue;
}

/*
	Check if the cube is moving. This is done by checking whether the gyro rotation values surpass a certain threshhold (gyroTollerance)
*/
bool isMoving() {
    int absGx = abs(gx);
    int absGy = abs(gy);
    int absGz = abs(gz);

    if ((absGx + absGy + absGz) <= gyroTollerance) {
        return false;
    } else {

        return true;
    }
}

/*
	Check whether the cube is laying flat on one side.
	Returns false when cube is laying flat on one side, false otherwise.
*/

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
