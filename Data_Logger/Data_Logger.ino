#include <Regexp.h>
#include <BH1750.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "RTClib.h"
#include "SdFat.h"


static volatile bool buttonClicks[4] = { false, false, false, false };
static volatile bool anyButtonClick = false;

class SDI_12 {
private:
  MatchState ms;
  Adafruit_BME680 bme;
  BH1750 lightMeter;
  const int SDI12PIN = 8;

  float SEALEVELPRESSURE_HPA = 1013.25;
  int address = 0;
  bool initialize = false;
  int continusMesurmentSensorID;

  void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency) {
    pmc_set_writeprotect(false);
    pmc_enable_periph_clk((uint32_t)irq);
    TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);
    uint32_t rc = (VARIANT_MCK / 128) * frequency;
    TC_SetRA(tc, channel, rc / 2);
    TC_SetRC(tc, channel, rc);
    TC_Start(tc, channel);
    tc->TC_CHANNEL[channel].TC_IER = TC_IER_CPCS;
    tc->TC_CHANNEL[channel].TC_IDR = ~TC_IER_CPCS;
    NVIC_EnableIRQ(irq);
  }

public:
  volatile bool measureFlag = false;

  void setUp() {
    Serial.begin(9600);
    Serial1.begin(1200, SERIAL_7E1);
    pinMode(SDI12PIN, OUTPUT);
    writeToSDI12("   ");
    Wire.begin();
  }

  void runCommand(char command[]) {
    Serial.println(command);
    ms.Target(command);

    if (ms.Match(".*?!") == REGEXP_MATCHED) {
      writeToSDI12(String(address));
    } else if (command[0] - '0' != address) {
      writeToSDI12("The address that you provided is not the current address.You can find the current address by using this command (?!) ");
      return;
    }

    // Change address
    if (ms.Match(".*[0-9]A[0-9]!") == REGEXP_MATCHED) {
      int newAddress = command[2] - '0';
      address = newAddress;
      writeToSDI12(String(address));
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
      if (!initialize) {
        if (!bme.begin(0x76)) {
          writeToSDI12("Error in the BME sensor");
        } else if (!lightMeter.begin()) {
          writeToSDI12("Error in the BH1750 sensor");
        } else {
          initialize = true;
          writeToSDI12("Measurement Started");
        }

      } else {
        writeToSDI12("You already initialized it");
      }

      bme.setTemperatureOversampling(BME680_OS_8X);
      bme.setHumidityOversampling(BME680_OS_2X);
      bme.setPressureOversampling(BME680_OS_4X);
      bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
      bme.setGasHeater(320, 150);  // 320*C for 150 ms
    }

    // Send Data
    if (ms.Match("^[0-9]+D[0-9]!") == REGEXP_MATCHED) {
      int commandAddress = command[0] - '0';
      int sensorID = command[2] - '0';

      if (commandAddress == address && initialize) {
        String value = String(address) + String(" ") + String(getData(sensorID));
        writeToSDI12(value);
      }
    }

    // Continuous Data
    if (ms.Match("^[0-9]+R[0-9]!") == REGEXP_MATCHED) {
      int commandAddress = command[0] - '0';
      int sensorID = command[2] - '0';

      if (commandAddress == address && initialize) {
        startTimer(TC0, 0, TC0_IRQn, 1);
        continusMesurmentSensorID = sensorID;
        Serial.println("Working");
      }
    }
  }

  float getData(int sensorID) {
    bme.performReading();
    float value = 0;

    switch (sensorID) {
      case 0:
        {
          value = bme.temperature;
          break;
        }
      case 1:
        {
          value = bme.humidity;
          break;
        }
      case 2:
        {
          value = bme.pressure / 100.0;
        }
      case 3:
        {
          value = bme.gas_resistance / 1000.0;
          break;
        }
      case 4:
        {
          value = bme.readAltitude(SEALEVELPRESSURE_HPA);
          break;
        }
      case 5:
        {
          value = lightMeter.readLightLevel();
          break;
        }
    }

    return value;
  }

  void writeToSDI12(String data) {

    digitalWrite(SDI12PIN, LOW);
    Serial1.println(data);
    Serial1.flush();

    // Clear any data that may have been echoed back
    while (Serial1.available() > 0) {
      char junk = Serial1.read();
    }

    delay(100);

    digitalWrite(SDI12PIN, HIGH);
  }

  void handleMeasurement() {
    if (measureFlag) {
      String value = String(address) + String(" ") + String(getData(continusMesurmentSensorID));
      writeToSDI12(value);
      Serial.println("Measurement taken and sent.");
      measureFlag = false;
    }
  }
};



class LCD_Display {
private:
  const int TFT_CS = 10;
  const int TFT_RST = 6;
  const int TFT_DC = 7;

  const int TFT_SCLK = 13;
  const int TFT_MOSI = 11;

  Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);


public:
  float sensorValues[5];
  int mappedSensorValues[5];
  RTC_DS1307 rtc;
  DateTime now;
  const int buttonPins[4] = { 2, 3, 4, 5 };
  bool fillRectNeeded = true;
  bool requedData = false;
  bool requedSaving = false;

  unsigned long last_interrupt_time = 0;
  unsigned long delayTime = 500;

  volatile int selectedOption = 0;
  volatile int pageNum = -1;

  char minValue[50];
  char maxValue[50];

  int upperValue;
  int lowerValue;

  char sensorName[50];

  

  void init() {
    for (int pin : buttonPins) {
      pinMode(pin, INPUT_PULLUP);
    }

    attachInterrupt(digitalPinToInterrupt(buttonPins[0]), goBack, RISING);
    attachInterrupt(digitalPinToInterrupt(buttonPins[1]), goDown, RISING);
    attachInterrupt(digitalPinToInterrupt(buttonPins[2]), goUp, RISING);
    attachInterrupt(digitalPinToInterrupt(buttonPins[3]), goFront, RISING);
    Serial.begin(9600);
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_BLACK);
    tft.setRotation(3);
    rtc.begin();

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  void setLayOut() {


    tft.drawLine(0, 13, 159, 13, ST77XX_WHITE);


    tft.drawLine(0, 103, 159, 103, ST77XX_WHITE);

    tft.fillCircle(14, 116, 8, ST77XX_GREEN);
    tft.drawLine(11.35, 115.65, 15.35, 119.65, ST77XX_BLACK);
    tft.drawLine(11.35, 116.65, 15.35, 120.65, ST77XX_BLACK);
    tft.drawLine(11.65, 116.65, 15.65, 112.65, ST77XX_BLACK);
    tft.drawLine(10.65, 116.65, 15.65, 111.65, ST77XX_BLACK);



    tft.fillCircle(146, 116, 8, ST77XX_GREEN);
    tft.drawLine(144.65, 113.35, 148.65, 117.35, ST77XX_BLACK);
    tft.drawLine(144.65, 112.35, 148.65, 116.35, ST77XX_BLACK);
    tft.drawLine(148.35, 116.35, 144.35, 120.35, ST77XX_BLACK);
    tft.drawLine(149.35, 116.35, 144.35, 121.35, ST77XX_BLACK);

    tft.fillCircle(58, 116, 8, ST77XX_GREEN);
    tft.fillTriangle(53, 117, 58, 122, 63, 117, ST77XX_BLACK);
    tft.fillTriangle(55, 117, 58, 120, 61, 117, ST77XX_GREEN);
    tft.fillRect(58.25, 111, 1.5, 7, ST77XX_BLACK);

    tft.fillCircle(102, 116, 8, ST77XX_GREEN);
    tft.fillTriangle(97, 115, 102, 110, 107, 115, ST77XX_BLACK);
    tft.fillTriangle(99, 115, 102, 112, 105, 115, ST77XX_GREEN);
    tft.fillRect(102.75, 114, 1.5, 7, ST77XX_BLACK);
  }

  void displayTime(DateTime now) {
    tft.setTextWrap(false);
    tft.setCursor(3, 3);
    tft.setTextSize(1);

    tft.setTextColor(ST77XX_WHITE);
    tft.print(now.year(), DEC);
    tft.print('/');
    tft.print(now.month(), DEC);
    tft.print('/');
    tft.print(now.day(), DEC);

    tft.setCursor(115, 3);
    tft.print(now.hour(), DEC);
    tft.print(':');
    tft.print(now.minute(), DEC);
    tft.print(':');
    tft.print(now.second(), DEC);
    tft.println();

    tft.setTextColor(ST77XX_BLACK);

    tft.setCursor(115, 3);
    tft.print(now.hour(), DEC);
    tft.print(':');
    tft.print(now.minute(), DEC);
    tft.print(':');
    tft.print(now.second(), DEC);
    tft.println();
  }


  void mainMenu() {
    if (fillRectNeeded) {
      tft.fillRect(0, 15, 160, 85, ST77XX_BLACK);
    }
    tft.setTextColor(ST77XX_BLUE);
    switch (selectedOption) {
      case 0:
        {
          tft.setCursor(15, 22);
          tft.println("> Temperature");
          break;
        }
      case 1:
        {
          tft.setCursor(15, 34);
          tft.println("> Humidity");
          break;
        }
      case 2:
        {
          tft.setCursor(15, 46);
          tft.println("> Pressure");
          break;
        }
      case 3:
        {
          tft.setCursor(15, 58);
          tft.println("> Gas Resistance");
          break;
        }
      case 4:
        {
          tft.setCursor(15, 70);
          tft.println("> Altutude");
          break;
        }
      case 5:
        {
          tft.setCursor(15, 82);
          tft.println("> Light Level");
          break;
        }
    }


    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(15, 22);
    tft.println("> Temperature");


    tft.setCursor(15, 34);
    tft.println("> Humidity");


    tft.setCursor(15, 46);
    tft.println("> Pressure");


    tft.setCursor(15, 58);
    tft.println("> Gas Resistance");


    tft.setCursor(15, 70);
    tft.println("> Altutude");


    tft.setCursor(15, 82);
    tft.println("> Light Level");


    fillRectNeeded = false;
  }

  void showGraph() {
    if (fillRectNeeded) {
      tft.fillRect(0, 15, 160, 85, ST77XX_BLACK);
    }

    switch (selectedOption) {
      case 0:
        {
          strcpy(sensorName, "Temperature (*C)");
          lowerValue = -40;
          upperValue = 85;
          strcpy(minValue, "0");
          strcpy(maxValue, "85");
          break;
        }
      case 1:
        {
          strcpy(sensorName, "Humidity (%)");
          lowerValue = 0;
          upperValue = 100;
          strcpy(minValue, "0");
          strcpy(maxValue, "100");
          break;
        }
      case 2:
        {
          strcpy(sensorName, "Pressure (hPa)");
          lowerValue = 0;
          upperValue = 1100;
          strcpy(minValue, "0");
          strcpy(maxValue, "1100");
          break;
        }
      case 3:
        {
          strcpy(sensorName, "Gas Resistance (KOhms)");
          lowerValue = 0;
          upperValue = 1000;
          strcpy(minValue, "0");
          strcpy(maxValue, "1K");
          break;
        }
      case 4:
        {
          strcpy(sensorName, "Altatude (m) in K");
          lowerValue = 0;
          upperValue = 9300;
          strcpy(minValue, "0");
          strcpy(maxValue, "9.3");
          break;
        }
      case 5:
        {
          strcpy(sensorName, "Light Sensor (lux) in K ");
          lowerValue = 0;
          upperValue = 100000;
          strcpy(minValue, "0");
          strcpy(maxValue, "100");
          break;
        }
    }

    tft.drawLine(25, 85.5, 45, 85.5 - mappedSensorValues[0], ST77XX_BLACK);
    tft.drawLine(45, 85.5 - mappedSensorValues[0], 65, 85.5 - mappedSensorValues[1], ST77XX_BLACK);
    tft.drawLine(65, 85.5 - mappedSensorValues[1], 85, 85.5 - mappedSensorValues[2], ST77XX_BLACK);
    tft.drawLine(85, 85.5 - mappedSensorValues[2], 105, 85.5 - mappedSensorValues[3], ST77XX_BLACK);
    tft.drawLine(105, 85.5 - mappedSensorValues[3], 125, 85.5 - mappedSensorValues[4], ST77XX_BLACK);

    for (int i = 0; i < 5; i++) {
      mappedSensorValues[i] = map(sensorValues[i], lowerValue, upperValue, 0, 50);
    }

    tft.drawLine(21.64, 25, 21.64, 92.52, ST77XX_WHITE);
    tft.drawLine(21.64, 25, 19, 30.46, ST77XX_WHITE);
    tft.drawLine(21.64, 25, 24.27, 30.46, ST77XX_WHITE);
    tft.drawLine(21.64, 92.52, 139, 92.52, ST77XX_WHITE);
    tft.drawLine(139, 92.52, 132.41, 88.06, ST77XX_WHITE);
    tft.drawLine(139, 92.52, 132.41, 96, ST77XX_WHITE);

    tft.setTextColor(ST77XX_WHITE);

    tft.setCursor(53, 20);
    tft.print(sensorName);

    tft.setCursor(1, 29);
    tft.print(maxValue);

    tft.setCursor(1, 80);
    tft.print(minValue);


    tft.setCursor(35, 92);
    tft.print("Press > to save Data");

    tft.drawLine(25, 85.5, 45, 85.5 - mappedSensorValues[0], ST77XX_WHITE);
    tft.drawLine(45, 85.5 - mappedSensorValues[0], 65, 85.5 - mappedSensorValues[1], ST77XX_WHITE);
    tft.drawLine(65, 85.5 - mappedSensorValues[1], 85, 85.5 - mappedSensorValues[2], ST77XX_WHITE);
    tft.drawLine(85, 85.5 - mappedSensorValues[2], 105, 85.5 - mappedSensorValues[3], ST77XX_WHITE);
    tft.drawLine(105, 85.5 - mappedSensorValues[3], 125, 85.5 - mappedSensorValues[4], ST77XX_WHITE);

    fillRectNeeded = false;
  }

  void startUp() {
    tft.setCursor(52, 82);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Data Logger");

    tft.fillRect(56, 38, 41, 31, ST77XX_WHITE);
    tft.fillRect(57, 39, 39, 22, ST77XX_BLACK);
    tft.fillRect(92, 42, 8, 6, ST77XX_BLACK);

    tft.drawLine(56.65, 56.65, 62.65, 50.65, ST77XX_WHITE);
    tft.drawLine(56.65, 56.65, 62.65, 50.65, ST77XX_BLACK);
    tft.fillCircle(64, 49, 3, ST77XX_WHITE);
    tft.fillCircle(64, 49, 3, ST77XX_BLACK);

    tft.drawLine(66, 50.5, 71.5, 55.5, ST77XX_WHITE);
    tft.drawLine(66, 50.5, 71.5, 55.5, ST77XX_BLACK);
    tft.fillCircle(73, 56, 3, ST77XX_WHITE);
    tft.fillCircle(73, 56, 3, ST77XX_BLACK);

    tft.drawLine(74.5, 53.5, 78.5, 48, ST77XX_WHITE);
    tft.drawLine(74.5, 53.5, 78.5, 48, ST77XX_BLACK);
    tft.fillCircle(79, 47, 3, ST77XX_WHITE);
    tft.fillCircle(79, 47, 3, ST77XX_BLACK);

    tft.drawLine(80.5, 48, 89, 56, ST77XX_WHITE);
    tft.drawLine(80.5, 48, 89, 56, ST77XX_BLACK);
    tft.fillCircle(89, 56, 3, ST77XX_WHITE);
    tft.fillCircle(89, 56, 3, ST77XX_BLACK);

    tft.drawLine(90.5, 55, 103, 40.5, ST77XX_WHITE);
    tft.drawLine(90.5, 55, 103, 40.5, ST77XX_BLACK);

    tft.drawLine(103, 40.5, 99, 40.5, ST77XX_WHITE);
    tft.drawLine(103, 40.5, 103.5, 44, ST77XX_WHITE);

    tft.drawLine(103, 40.5, 99, 40.5, ST77XX_BLACK);
    tft.drawLine(103, 40.5, 103.5, 44, ST77XX_BLACK);





    tft.fillRect(56, 38, 41, 31, ST77XX_WHITE);
    tft.fillRect(57, 39, 39, 22, ST77XX_BLACK);
    tft.fillRect(92, 42, 8, 6, ST77XX_BLACK);

    tft.drawLine(56.65, 56.65, 62.65, 50.65, ST77XX_WHITE);
    tft.fillCircle(64, 49, 3, ST77XX_WHITE);


    tft.drawLine(66, 50.5, 71.5, 55.5, ST77XX_WHITE);
    tft.fillCircle(73, 56, 3, ST77XX_WHITE);


    tft.drawLine(74.5, 53.5, 78.5, 48, ST77XX_WHITE);
    tft.fillCircle(79, 47, 3, ST77XX_WHITE);


    tft.drawLine(80.5, 48, 89, 56, ST77XX_WHITE);
    tft.fillCircle(89, 56, 3, ST77XX_WHITE);


    tft.drawLine(90.5, 55, 103, 40.5, ST77XX_WHITE);

    tft.drawLine(103, 40.5, 99, 40.5, ST77XX_WHITE);
    tft.drawLine(103, 40.5, 103.5, 44, ST77XX_WHITE);




    tft.fillRect(72, 71, 9, 3, ST77XX_WHITE);
    tft.fillRect(67, 74, 19, 4, ST77XX_WHITE);
  }

  void savingSimbol() {
    tft.setCursor(92, 1);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("(S)");

    tft.setCursor(92, 1);
    tft.setTextColor(ST77XX_BLACK);
    tft.print("(S)");
  }


  void handleDisplay() {

    now = rtc.now();
    displayTime(now);

    if (millis() - last_interrupt_time > delayTime && anyButtonClick) {

      if (buttonClicks[0]) {
        pageNum = (pageNum + 1) % 3;
        buttonClicks[0] = false;
        fillRectNeeded = true;
      }

      if (buttonClicks[1]) {
        selectedOption = (selectedOption - 1 + 6) % 6;
        buttonClicks[1] = false;
      }

      if (buttonClicks[2]) {
        selectedOption = (selectedOption + 1) % 6;
        buttonClicks[2] = false;
      }

      if (buttonClicks[3]) {
        if (pageNum != 0) {
          pageNum = (pageNum - 1 + 3) % 3;
          buttonClicks[3] = false;
          fillRectNeeded = true;
          Serial.print("Page Nu,");
          Serial.println(pageNum);
        }
      }
      last_interrupt_time = millis();
      anyButtonClick = false;
    }


    if (pageNum == 0) {
      mainMenu();
    } else if (pageNum == 1) {
      requedData = true;
      showGraph();
    } else if (pageNum == 2) {
      requedData = true;
      requedSaving = true;
      showGraph();
      savingSimbol();
    }
  }

  static void goBack() {

    buttonClicks[0] = true;
    anyButtonClick = true;
  }

  static void goDown() {

    buttonClicks[1] = true;
    anyButtonClick = true;
  }

  static void goUp() {
    buttonClicks[2] = true;
    anyButtonClick = true;
  }

  static void goFront() {
    buttonClicks[3] = true;
    anyButtonClick = true;
  }

};


class Data_Logger {
private:
  uint8_t SD_CS_PIN = A3;
  static constexpr uint8_t SOFT_MISO_PIN = 12;
  static constexpr uint8_t SOFT_MOSI_PIN = 11;
  static constexpr uint8_t SOFT_SCK_PIN = 13;
  SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)

  SdFs sd;
  FsFile file;

  LCD_Display display;
  bool displayState = false;
  int button1State = 0;
  int button2State = 0;
  int prevButton1State = 0;
  int prevButton2State = 0;
public:
  SDI_12 sdi12;

  void init() {
    sdi12.setUp();
    if (!sd.begin(SD_CONFIG)) {
      Serial.println("SD card initialization failed!");
      return;
    }
    if (!file.open("Sensor_Data.txt", O_RDWR | O_CREAT)) {
      Serial.println(F("open failed"));
    }

    file.close();
    display.init();
    display.startUp();
    display.setLayOut();
  }

  void displayStat() {
    button1State = digitalRead(display.buttonPins[0]);
    button2State = digitalRead(display.buttonPins[1]);

    if (button1State == LOW && button2State == LOW) {
      if (prevButton1State == HIGH || prevButton2State == HIGH) {
        displayState ^= true;
      }
    }
    prevButton1State = button1State;
    prevButton2State = button2State;
  }

  void handleLogger() {

    if (Serial1.available()) {
      String input = Serial1.readStringUntil('\n');
      char buf[input.length()];
      memset(buf, 0, sizeof(buf));


      for (int i = 1; i < input.length(); i++) {
        buf[i - 1] = input[i];
      }

      sdi12.runCommand(buf);
    }
    sdi12.handleMeasurement();

    displayStat();


    if (!displayState) return;
    display.handleDisplay();

    if (display.requedData) {
      for (int i = 0; i < 5; i++) {
        display.sensorValues[i] = sdi12.getData(display.selectedOption);
      }
      display.requedData = false;
    }

    if (display.requedSaving) {
      saveData(file, display.sensorValues, display.sensorName, display.now);
      display.requedSaving = false;
    }
  }

  void saveData(FsFile file, float sensorValues[5], char sensorName[50], DateTime now) {
    file.open("Sensor_Data.txt", O_RDWR);
    Serial.println("Woring");
    //file.rewind();
    file.seekEnd();
    file.println("-----------------------------------------------------------------------------------------------------------------------------------------------------");
    file.println(sensorName);
    file.println("-----------------------------------------------------------------------------------------------------------------------------------------------------");
    for (int i = 0; i < 5; i++) {
      file.print("Date : ");
      file.print(now.year());
      file.print("/");
      file.print(now.month());
      file.print("/");
      file.print(now.day());
      file.print("  -  ");
      file.print(now.hour());
      file.print(":");
      file.print(now.minute());
      file.print(":");
      file.print(now.second());
      file.print(" | Value : ");
      file.print(sensorValues[i]);
      file.print(" | ");
      file.println();
    }

    file.close();
  }
};

Data_Logger logger;

void setup() {
  logger.init();
}


void loop() {
  logger.handleLogger();
}


void TC0_Handler() {
  TC_GetStatus(TC0, 0);
  logger.sdi12.measureFlag = true;
}
