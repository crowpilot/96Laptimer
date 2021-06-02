//Clock and Laptimer(stop watch)
//_mode==0 clock
//_mode==1 or 2 Laptimer

#ifndef Watch_h
#define Watch_h
#include<M5Stack.h>

class Watch {
  public:
    Watch(int x,int y,int w,int h);
  void setCountPos(int countX,int countY);//w h not support

    void displayWatch();

    bool isClock();
    void toggleLap();
    String getButton();

    void clockMode();
    void lapMode();
    
    void startLap();
    void stopLap();
    unsigned long lapTime();
    void resetLap();
    void displayLap();
    void grid();

    void clockAdjust(int hours, int minutes, int seconds);
    void refreshClock();
    void displayClock();
    int getHours();
    int getMinutes();
    int getSeconds();

    void resetDisplay();

  private:
    unsigned long _startMillis=0;
    unsigned long _clockMillis=0;
    unsigned long _lastLap[10];
  unsigned long _fastest=0;
    int _hours;
    int _minutes;
    int _seconds;
    int _x,_y,_w,_h;
  int _countX,_countY;
    uint8_t _mode=0;//0:clock 1:lap(stop) 2:lap(started)

  int _lapCount=0;
};
#endif
