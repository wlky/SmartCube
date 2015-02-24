#include "MPU6050.h"
#include "I2Cdev.h"


int getOrientation(int _ax, int _ay, int _az);
int getRotationValue(int _orientation, int _gx, int _gy, int _gz);
void switchMode(int _orientation);
int notification(String _type);
void setVolume(int _volume);
void setColor(int _r, int _g, int _b);
void setIsPulsing(bool _isPulsing);
bool getIsPulsing();
void pulse();
bool inMotion();
void setBrightness(float _brightness);
void setDimmValue(float _rotationValue);
float getDimmValue();
int getMax(int a, int b, int c);
void makeSound();
void makeNotificationSound();
void switchModeLocal(int _orientation);
void sendCommand(String command);
void resetSleepTimer();
unsigned long sendColor(unsigned int r, unsigned int g, unsigned int b);
bool isMoving();
unsigned long updateColor();
void showModes();
void checkTouchEvents();
void checkForDockingstation();
void transitionToMode(int _orientation, int timeMillis);