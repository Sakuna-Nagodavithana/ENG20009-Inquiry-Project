#include <Regexp.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

class SDI_12 {
private:
    MatchState ms;
    int BME_SCK = 13;
    int BME_MISO = 12;
    int BME_MOSI = 11;
    int BME_CS = 10;
    float SEALEVELPRESSURE_HPA = 1013.25;
    Adafruit_BME680 bme; // I2C
    int address = 0;
    bool initialize = false;

public:
    void runCommand(String command) {
        char commandChars[command.length() + 1];
        command.toCharArray(commandChars, command.length() + 1);
        ms.Target(commandChars);

        // Address Query
        if (ms.Match("^\\?!") == REGEXP_MATCHED) {
            writeToSDI12(String(address));
        }

        // Change address
        if (ms.Match("^[0-9A-Z]+[0-9A-Z]{1,2}") == REGEXP_MATCHED) {
            int newAddress = command[0] - '0';
            int newAddressLength = command[1] - '0';

            if (newAddress == address) {
                address = (newAddressLength == 2 ? (command[2] - '0') * 10 + (command[3] - '0') : command[2] - '0');
                writeToSDI12(String(address));
            } else {
                writeToSDI12("The address that you provided is not the current address. You can find the current address by using this command (?!)");
            }
        }

        // Send Identification
        if (ms.Match("^[0-9A-Z]+I!") == REGEXP_MATCHED) {
            writeToSDI12(String(address));
            writeToSDI12("14");
            writeToSDI12("ENG2009");
            writeToSDI12("104643522");
        }

        // Start Measurement
        if (ms.Match("^[0-9A-Z]+M!") == REGEXP_MATCHED) {
            bme.setTemperatureOversampling(BME680_OS_8X);
            bme.setHumidityOversampling(BME680_OS_2X);
            bme.setPressureOversampling(BME680_OS_4X);
            bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
            bme.setGasHeater(320, 150); // 320*C for 150 ms

            if (!initialize) {
                bme.begin(0x76);
                initialize = true;
                writeToSDI12("Measurement Started");
            } else {
                writeToSDI12("You already initialized it");
            }
        }

        // Send Data
        if (ms.Match("^[0-9A-Z]+D[0-9]!") == REGEXP_MATCHED) {
            int commandAddress = command[0] - '0';
            int sensorID = command[2] - '0';

            if (commandAddress != address) {
                writeToSDI12("The address that you provided is not the current address. You can find the current address by using this command (?!)");
            } else {
                float value = getData(sensorID);
                writeToSDI12(String(value));
            }
        }
    }

    float getData(int sensorID) {
        bme.performReading();
        float value = 0;

        if (sensorID == 0) {
            value = bme.temperature;
        } else if (sensorID == 1) {
            value = bme.pressure / 100.0;
        } else if (sensorID == 2) {
            value = bme.humidity;
        } else if (sensorID == 3) {
            value = bme.gas_resistance / 1000.0;
        } else if (sensorID == 4) {
            value = bme.readAltitude(SEALEVELPRESSURE_HPA);
        }

        return value;
    }

    void writeToSDI12(String data) {
        // DIRO Pin LOW to Send to SDI-12
        digitalWrite(7, LOW);
        Serial1.println(data);
        // HIGH to Receive from SDI-12
        digitalWrite(7, HIGH);
    }
};

SDI_12 sdi12;
MatchState ms2;

void setup() {
    Serial.begin(9600);
    Serial1.begin(1200, SERIAL_7E1);
    pinMode(7, OUTPUT); 
    //DIRO Pin LOW to Send to SDI-12
    digitalWrite(7, LOW); 
    Serial1.println("HelloWorld");
    delay(100);

    //HIGH to Receive from SDI-12
    digitalWrite(7, HIGH); 
}

void loop() {
    
    if (Serial.available()) {
    String input = Serial1.readStringUntil('\n');
    char buf[input.length() + 1];
    input.toCharArray(buf, input.length() + 1);
    ms2.Target(buf);
    Serial.println(input);
    if (ms2.Match(".+\?!") == REGEXP_MATCHED) {
      Serial.println("working");
    } else {
      Serial.println("Invalid input");
    }
    Serial.println("Enter a string:");
  }

}