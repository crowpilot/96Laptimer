//IMU must define before M5stack.h
//M5stack gray old
//#define M5STACK_MPU9250
#define M5STACK_MPU6886

#include<M5Stack.h>
#include<Wire.h>

#include "src/Dashboard.h"
#include "src/Graph.h"
#include "src/Ins.h"
#include "src/Watch.h"
#include "src/httpAP.h"
#include "src/logCSV.h"
//gps
#include <TinyGPS++.h>
//3G
//#define TINY_GSM_MODEM_UBLOX
//#include <TinyGsmClient.h>
//LED neopixel
#include<Adafruit_NeoPixel.h>
//ENV2
#include<Adafruit_BMP280.h>
#include<Adafruit_SHT31.h>

//GPS
TinyGPSPlus gps;
HardwareSerial GPSserial(2);

//3G
//TinyGsm modem(Serial2);
//TinyGsmClient ctx(modem);

//LED
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(10, 15, NEO_GRB + NEO_KHZ800);

//IMU
Ins ins = Ins();

//ENV2
Adafruit_BMP280 bme;
Adafruit_SHT31 sht30 = Adafruit_SHT31();

//Clock and Laptime
Dashboard dashboard;
Watch watch = Watch(30, 159, 310, 50);


//http server

//logging
logCSV logcsv;

//xTask
//void mainLoop(void *arg);
void refreshIMU(void* arg);
void refreshIMUGraph(void* arg);
void refreshENV(void* arg);
void refreshClock(void* arg);
void writeData(void* arg);
void wifiServer(void* arg);

TaskHandle_t xHandleWriteData;
TaskHandle_t xHandleWifiServer;
SemaphoreHandle_t xMutex = NULL;

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Power.begin();
  Wire.begin();

  //GPS
  GPSserial.begin(9600, SERIAL_8N1, 36, 26);
  //GPSserial.begin(9600);

  //watch
  watch.setCountPos(30,90);
  watch.lapMode();

  //LED
  pixels.begin();

  //IMU
  ins.init();
  
  dashboard.createMeter(30,50,25);


  //ENV2
  bme.begin(0x76);
  sht30.begin(0x44);

  //clock box
  //dashboard.init();
  dashboard.bottomButton(String("clock"), String("LAP"), String("Data"));

  //task
  //if (M5.Power.isCharging()) {
    xMutex = xSemaphoreCreateMutex();
    //xTaskCreatePinnedToCore(mainLoop, "mainLoop", 8192, NULL, 10, NULL, 1);
    //xTaskCreatePinnedToCore(refreshENV, "ENV", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(refreshIMU, "IMU", 8192, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(refreshIMUGraph, "IMU", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(writeData, "writeData", 8192, NULL, 2, &xHandleWriteData, 0);
  //}
}



void loop() {
  //mode 0:clock 1:laptimer 2:laptimer running
  //refreshServer();
  xSemaphoreTake(xMutex, portMAX_DELAY);


  //header 3G connection GPS
  //M5.Lcd.print(gps.satellites.value());
  /*if (modem.isNetworkConnected()) {
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("3G");
    }*/

  //LED
  pixels.setPixelColor(0, pixels.Color(0, 100, 0));
  pixels.show();

  watch.displayWatch();

  //button setting
  M5.update();
  if (M5.BtnA.wasPressed()) {
    //clock mode
    watch.resetDisplay();
    watch.clockMode();
    dashboard.bottomButton(String("clock"), String("LAP"), String("Data"));
  }
  if (M5.BtnB.wasPressed()) {
    //lap mode start stop
    if (watch.isClock()) {
      watch.resetDisplay();
      watch.lapMode();
      dashboard.bottomButton(String("clock"), watch.getButton(), String("RESET"));
    } else {
      //watch.toggleLap();
      watch.grid();
      dashboard.bottomButton(String("clock"), watch.getButton(), String("RESET"));
    }
  }
  if (M5.BtnC.wasPressed()) {
    if (watch.isClock()) {
      //lap to wifi mode
      dashboard.bottomButton(String(""), String(""), String(""));
      vTaskSuspend(xHandleWriteData);
      xTaskCreatePinnedToCore(wifiServer, "writeData", 8192, NULL, 3, &xHandleWifiServer, 0);
    } else {
      watch.stopLap();
      watch.resetLap();
    }
  }

  xSemaphoreGive(xMutex);
  vTaskDelay(1);
}

//TASK
void refreshIMU(void* arg) {
  //IMU display task
  for (;;) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    ins.reload();
    xSemaphoreGive(xMutex);
    vTaskDelay(5);
  }
}
void refreshIMUGraph(void* arg) {
  //bank graph and G graph
  for (;;) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    dashboard.updateMeter(ins.roll());
    //test value display

    xSemaphoreGive(xMutex);
    vTaskDelay(100);
  }
}

void refreshENV(void* arg) {
  //show ENV sensor value
  static float tmp = 0.0F;
  static float hum = 0.0F;
  static float pressure = 0.0F;
  for (;;) {
    tmp = sht30.readTemperature();
    hum = sht30.readHumidity();
    pressure = bme.readPressure() * 0.0002953;

    xSemaphoreTake(xMutex, portMAX_DELAY);

    xSemaphoreGive(xMutex);
    vTaskDelay(10000);
  }
}

void writeData(void* arg) {
  //core 0
  //write CSV log to SD card
  //float yaw, roll, pitch, temp;
  for (;;) {
    while (GPSserial.available()) {
      //adjust clock
      gps.encode(GPSserial.read());
      if (gps.time.minute()) {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        watch.clockAdjust(gps.time.hour(), gps.time.minute(), gps.time.second());
        logcsv.setTime(gps.date.year(), gps.date.month(), gps.date.day(), watch.getHours(), watch.getMinutes(), watch.getSeconds());
        xSemaphoreGive(xMutex);
      }
    }
    if (gps.satellites.value()) {

    }
    logcsv.setGPS(gps.location.lat(), gps.location.lng());

    xSemaphoreTake(xMutex, portMAX_DELAY);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(gps.satellites.value());
    //M5.Lcd.print(M5.Power.getBatteryLevel());
    logcsv.setAHRS(ins.pitch(), ins.roll(), ins.yaw());
    logcsv.setG(ins.accelG(), 0);

    if (watch.isClock()) {
      logcsv.setInterval(1000);//1hz
    } else {
      logcsv.setInterval(100);//10hz
    }
    xSemaphoreGive(xMutex);

    //write SD
    if(gps.date.month()){
      logcsv.writeCSV();
    }else if(logcsv.getInterval()==100){
      logcsv.writeCSV();
    }

    vTaskDelay(logcsv.getInterval());
  }
}

void wifiServer(void* arg) {
  //core 0
  //stop all
  //start wifi server
  xSemaphoreTake(xMutex, portMAX_DELAY);
  initServer();
  dashboard.bottomButton(String(""), WiFi.softAPIP().toString(), String(""));
  xSemaphoreGive(xMutex);
  for (;;) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    refreshServer();
    xSemaphoreGive(xMutex);
    vTaskDelay(1);
  }
}
