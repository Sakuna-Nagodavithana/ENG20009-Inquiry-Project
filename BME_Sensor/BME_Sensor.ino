#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

class BME_Sensor {
private:
    int BME_SCK = 13;
    int BME_MISO = 12;
    int BME_MOSI = 11;
    int BME_CS = 10;
    float SEALEVELPRESSURE_HPA = 1013.25;
    Adafruit_BME680 bme; // I2C
    bool started = false;

public:
    bool startMeasurement() {
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(320, 150); // 320*C for 150 ms

        if (!started) {
            bme.begin(0x76);
            started = true;
            return true;
        }
        return false;
    }

    float getData(int sensorID) {
        bme.performReading();
        float value = 0;
        
        if (sensorID == 0) {
            value = bme.readTemperature() ;
        }
        if (sensorID == 1) {
            value = bme.readPressure() / 100.0;
        }
        if (sensorID == 2) {
            value = bme.readHumidity() ;
        }
        if (sensorID == 3) {
            value = bme.readGas() / 1000.0;
        }
        if (sensorID == 4) {
            value = bme.readAltitude(SEALEVELPRESSURE_HPA);
        }
        return value;
    }
};


BME_Sensor sensor ;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  sensor.startMeasurement();
}

void loop() {
  Serial.println(sensor.getData(0));
  // put your main code here, to run repeatedly:

}
