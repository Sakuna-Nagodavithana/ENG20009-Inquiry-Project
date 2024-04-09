#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"


class BME_Sensor{
  private:
    const BME_SCK 13
    const BME_MISO 12
    const BME_MOSI 11
    const BME_CS 10
    const SEALEVELPRESSURE_HPA (1013.25)
    Adafruit_BME680 bme; // I2C
    bool started = false;
  public:
    BME_Sensor() {
      // Set up oversampling and filter initialization
      bme.setTemperatureOversampling(BME680_OS_8X);
      bme.setHumidityOversampling(BME680_OS_2X);
      bme.setPressureOversampling(BME680_OS_4X);
      bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
      bme.setGasHeater(320, 150); // 320*C for 150 ms
    }

    bool startMeasurement(){
      if(!started){
        bme.begin();
        started = true;
        return true;
      }

      return false;
    }

    int getData(int sensorID){
      int valuse;
      if(sensorID == 0){
        value = bme.readTemperature()
      }

      if(sensorID == 1){
        value = bme.readTemperature()
      }

      if(sensorID == 2){
        value = bme.readTemperature()
      }

      if(sensorID == 3){
        value = bme.readTemperature()
      }

      if(sensorID == 4){
        value = bme.readTemperature()
      }
    }

}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
