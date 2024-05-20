///////////////////////////////////////////////////
// ENG20009 Inquiry Project - SDI-12 Data Logger //
// David Micallef                                //
// Sakuna Nagodavithana                          //
// Vichitra Dias                                 //
// Last Edit: 2024/05/17                         //
///////////////////////////////////////////////////

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
  int continusMesurmentSensorID;

  bool initialize = false;

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

  float dataValue[6];

  void setUp() {
    Serial.begin(9600);
    Serial1.begin(1200, SERIAL_7E1);
    pinMode(SDI12PIN, OUTPUT);
    writeToSDI12("   ");
    Wire.begin();
    startTimer(TC1, 0, TC3_IRQn, 1);
  }

  void runCommand(char command[]) {
    Serial.println(command);
    ms.Target(command);

    // Check Address
    if (ms.Match("?!") == REGEXP_MATCHED) {
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
      String initilizationString = String(address) + String("14") + String("ENG20009") + String("104643522");
      writeToSDI12(initilizationString);
    }

    // Start Measurement
    if (ms.Match("^[0-9A-Z]+M!") == REGEXP_MATCHED) {
      int startTime;
      if (!initialize) {
        startTime = millis();
        if (!bme.begin(0x76)) {
          writeToSDI12("Error in the BME sensor");
        } else if (!lightMeter.begin()) {
          writeToSDI12("Error in the BH1750 sensor");
        } else {
          bme.setTemperatureOversampling(BME680_OS_8X);
          bme.setHumidityOversampling(BME680_OS_2X);
          bme.setPressureOversampling(BME680_OS_4X);
          bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
          bme.setGasHeater(320, 150);  // 320*C for 150 ms
          String startMesurementString = String(address)+String((millis() - startTime)/1000)+String((millis() - startTime)/100) + String("6");
          writeToSDI12(startMesurementString);
          initialize = true;
        }



      } else {
        writeToSDI12("You already initialized it");
      }
    }

    // Send Data
    if (ms.Match("^[0-9]+D[0-9]!") == REGEXP_MATCHED) {
      int commandAddress = command[0] - '0';
      int sensorID = command[2] - '0';
      String value;
      getData();
      if (commandAddress == address && initialize) {
        if (sensorID == 0) {
          value = String(address) + String("+") + String(dataValue[0]) + String("+") + String(dataValue[1]) + String("+") + String(dataValue[2]) + String("+") + String(dataValue[3]) + String("+") + String(dataValue[4]);
        } else if (sensorID == 1) {
          value = String(address) + String("+") + String(dataValue[5]);
        }
        writeToSDI12(value);
      }
    }

    // Continuous Data
    if (ms.Match("^[0-9]+R[0-9]!") == REGEXP_MATCHED) {
      int commandAddress = command[0] - '0';
      int sensorID = command[2] - '0';

      if (commandAddress == address && initialize) {
        startTimer(TC0, 0, TC0_IRQn, 2);
        continusMesurmentSensorID = sensorID;
        //Serial.println("Working");
      }
    }
  }

  // Get data from sensors and save to temp array
  void getData() {
    bme.performReading();
    dataValue[0] = bme.temperature;
    dataValue[1] = bme.humidity;
    dataValue[2] = bme.pressure / 100.0;
    dataValue[3] = bme.gas_resistance / 1000.0;
    dataValue[4] = bme.readAltitude(SEALEVELPRESSURE_HPA);
    dataValue[5] = lightMeter.readLightLevel();
  }

  // Send text to terminal via SDI12 connection
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

  // Get data from sensors and display in terminal - used for Continuous Measurement
  void handleMeasurement() {
    getData();
    String value;
    if (continusMesurmentSensorID == 0) {
      value = String(address) + String("+") + String(dataValue[0]) + String("+") + String(dataValue[1]) + String("+") + String(dataValue[2]) + String("+") + String(dataValue[3]) + String("+") + String(dataValue[4])+ String("+") + String(dataValue[5]);
    } 
    writeToSDI12(value);
    Serial.println("Measurement taken and sent.");
  }
};


class LCD_Display {
private:
  const int TFT_CS = 10;
  const int TFT_RST = 6;
  const int TFT_DC = 7;
  const int TFT_SCLK = 13;
  const int TFT_MOSI = 11;
  const int buttonPins[4] = { 2, 3, 4, 5 };

  Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
  RTC_DS1307 rtc;

  char minValue[50];
  char maxValue[50];
  char sensorName[50];

  int upperValue;
  int lowerValue;

  bool fillRectNeeded = true;

public:
  float sensorValues[6][20];
  int mappedSensorValues[6][20];

  DateTime now;

  bool requedSaving = false;
  bool displayChangeFlag = false;
  bool clockRefreshFlag = false;
  bool menuStarted = false;

  volatile int selectedOption = 0;
  volatile int pageNum = -1;


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
    // Draws basic layout, navigation buttons, and date

    // Date
    now = rtc.now();
    tft.setCursor(3, 3);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(now.timestamp(DateTime::TIMESTAMP_DATE));

    tft.drawLine(0, 13, 159, 13, ST77XX_WHITE);    // Top Line
    tft.drawLine(0, 103, 159, 103, ST77XX_WHITE);  // Bottom Line

    // Left arrow symbol
    tft.fillCircle(14, 116, 8, ST77XX_GREEN);
    tft.drawLine(11.35, 115.65, 15.35, 119.65, ST77XX_BLACK);
    tft.drawLine(11.35, 116.65, 15.35, 120.65, ST77XX_BLACK);
    tft.drawLine(11.65, 116.65, 15.65, 112.65, ST77XX_BLACK);
    tft.drawLine(10.65, 116.65, 15.65, 111.65, ST77XX_BLACK);

    // Down Arrow Symbol
    tft.fillCircle(58, 116, 8, ST77XX_GREEN);
    tft.fillTriangle(53, 117, 58, 122, 63, 117, ST77XX_BLACK);
    tft.fillTriangle(55, 117, 58, 120, 61, 117, ST77XX_GREEN);
    tft.fillRect(58.25, 111, 1.5, 7, ST77XX_BLACK);

    // Up Arrow Symbol
    tft.fillCircle(102, 116, 8, ST77XX_GREEN);
    tft.fillTriangle(97, 115, 102, 110, 107, 115, ST77XX_BLACK);
    tft.fillTriangle(99, 115, 102, 112, 105, 115, ST77XX_GREEN);
    tft.fillRect(102.75, 114, 1.5, 7, ST77XX_BLACK);

    // Right Arrow Symbol
    tft.fillCircle(146, 116, 8, ST77XX_GREEN);
    tft.drawLine(144.65, 113.35, 148.65, 117.35, ST77XX_BLACK);
    tft.drawLine(144.65, 112.35, 148.65, 116.35, ST77XX_BLACK);
    tft.drawLine(148.35, 116.35, 144.35, 120.35, ST77XX_BLACK);
    tft.drawLine(149.35, 116.35, 144.35, 121.35, ST77XX_BLACK);
  }

  void displayTime(DateTime now, uint16_t color) {
    // Prints the time in the selected color

    tft.setTextWrap(false);
    tft.setTextSize(1);
    tft.setTextColor(color);
    tft.setCursor(113, 3);
    tft.print(now.timestamp(DateTime::TIMESTAMP_TIME));
  }

  void mainMenu() {
    // Displays menu options

    if (fillRectNeeded) clearDisplay();

    // Print Menu Options
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(15, 20);
    tft.println("> Temperature");
    tft.setCursor(15, 32);
    tft.println("> Humidity");
    tft.setCursor(15, 44);
    tft.println("> Pressure");
    tft.setCursor(15, 56);
    tft.println("> Gas Resistance");
    tft.setCursor(15, 68);
    tft.println("> Altutude");
    tft.setCursor(15, 80);
    tft.println("> Light Level");
    tft.setCursor(15, 92);
    tft.print("> Saving data ");
    if (!requedSaving) {
      savingSymbol(ST77XX_BLACK);
      tft.fillRect(129, 91, 18, 9, ST77XX_BLACK);
      tft.fillRect(106, 91, 13, 9, ST77XX_RED);
    }
    if (requedSaving) {
      savingSymbol(ST77XX_WHITE);
      tft.fillRect(106, 91, 13, 9, ST77XX_BLACK);
      tft.fillRect(129, 91, 18, 9, ST77XX_RED);
    }
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(107, 92);
    tft.print("No");
    tft.setCursor(130, 92);
    tft.print("Yes");

    tft.setTextColor(ST77XX_RED);
    switch (selectedOption) {
      // Highlights current menu selection by printing in RED
      case 0:
        {
          tft.setCursor(15, 20);
          tft.println("> Temperature");
          break;
        }
      case 1:
        {
          tft.setCursor(15, 32);
          tft.println("> Humidity");
          break;
        }
      case 2:
        {
          tft.setCursor(15, 44);
          tft.println("> Pressure");
          break;
        }
      case 3:
        {
          tft.setCursor(15, 56);
          tft.println("> Gas Resistance");
          break;
        }
      case 4:
        {
          tft.setCursor(15, 68);
          tft.println("> Altutude");
          break;
        }
      case 5:
        {
          tft.setCursor(15, 80);
          tft.println("> Light Level");
          break;
        }
      case 6:
        {
          tft.setCursor(15, 92);
          tft.print("> Saving data");
          break;
        }
    }
  }

  void clearDisplay() {
    // Clears full display screen; used for changing between pages
    fillRectNeeded = false;
    tft.fillRect(0, 15, 160, 87, ST77XX_BLACK);
  }

  void showGraph() {
    // Displays the graph for the current selected sensor
    if (fillRectNeeded) clearDisplay();

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
          strcpy(sensorName, "Light Sensor (lux)");
          lowerValue = 0;
          upperValue = 65535;
          strcpy(minValue, "0");
          strcpy(maxValue, "65k");
          break;
        }
    }

    // Erase Old displayed Data
    for (int i = 1; i < 20; i++) {
      int xOne = ((i - 1) * 6) + 25;
      int xTwo = (i * 6) + 25;
      tft.drawLine(xOne, 85.5 - mappedSensorValues[selectedOption][i - 1], xTwo, 85.5 - mappedSensorValues[selectedOption][i], ST77XX_BLACK);
      tft.fillCircle(xOne, 85.5 - mappedSensorValues[selectedOption][i - 1], 1, ST77XX_BLACK);
      tft.fillCircle(xTwo, 85.5 - mappedSensorValues[selectedOption][i], 1, ST77XX_BLACK);
    }

    // Map new data
    for (int i = 0; i < 20; i++) {
      mappedSensorValues[selectedOption][i] = map(sensorValues[selectedOption][i], lowerValue, upperValue, 0, 50);
    }

    // Draw XY axis
    tft.drawLine(21.64, 25, 21.64, 92.52, ST77XX_WHITE);
    tft.drawLine(21.64, 25, 19, 30.46, ST77XX_WHITE);
    tft.drawLine(21.64, 25, 24.28, 30.46, ST77XX_WHITE);
    tft.drawLine(21.64, 92.52, 149, 92.52, ST77XX_WHITE);
    tft.drawLine(151, 92.52, 145.54, 89.88, ST77XX_WHITE);
    tft.drawLine(151, 92.52, 145.54, 95.16, ST77XX_WHITE);

    // Label XY axis
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(53, 20);
    tft.print(sensorName);
    tft.setCursor(1, 29);
    tft.print(maxValue);
    tft.setCursor(1, 80);
    tft.print(minValue);
    // tft.setCursor(5,94);
    // tft.print("Press > for Data Saving");

    // Draw new displayed data
    for (int i = 1; i < 20; i++) {
      int xOne = ((i - 1) * 6) + 25;
      int xTwo = (i * 6) + 25;
      tft.drawLine(xOne, 85.5 - mappedSensorValues[selectedOption][i - 1], xTwo, 85.5 - mappedSensorValues[selectedOption][i], ST77XX_WHITE);
      tft.fillCircle(xOne, 85.5 - mappedSensorValues[selectedOption][i - 1], 1, ST77XX_RED);
      tft.fillCircle(xTwo, 85.5 - mappedSensorValues[selectedOption][i], 1, ST77XX_RED);
    }
  }

  void startUp() {
    // Displays Datalogger logo
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextWrap(false);
    tft.setTextSize(1);
    tft.setCursor(52, 82);
    tft.print("Data Logger");
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

  void savingSymbol(uint16_t color) {
    // Displays the saving symbol in selected color
    tft.setCursor(88, 3);
    tft.setTextColor(color);
    tft.print("(S)");
  }

  void displayStart() {
    // Waits for two-button input before going to main menu
    if (buttonClicks[0] == true && buttonClicks[3] == true) {
      displayChangeFlag = true;
      menuStarted = true;
      buttonClicks[0] = false;
      buttonClicks[3] = false;
    }
  }

  void refreshClock() {
    // Refreshes the displayed clock
    clockRefreshFlag = false;
    displayTime(now, ST77XX_BLACK);
    now = rtc.now();
    displayTime(now, ST77XX_WHITE);
  }

  void handleButtonClick() {
    // Processes button functions depending on page displayed and menu option selected
    if (anyButtonClick) {
      anyButtonClick = false;
      if (buttonClicks[0]) {
        buttonClicks[0] = false;
        if (pageNum == 0 && selectedOption == 6) {
          if (requedSaving == true) return;
          requedSaving = true;
          displayChangeFlag = true;
          return;
        }
        if (pageNum == 1) {
          requedSaving = !requedSaving;
          displayChangeFlag = true;
          return;
        }
        pageNum = (pageNum + 1) % 2;
        displayChangeFlag = true;
        fillRectNeeded = true;
      }

      if (buttonClicks[1]) {
        buttonClicks[1] = false;
        if (pageNum != 0) return;
        displayChangeFlag = true;
        selectedOption = (selectedOption - 1 + 7) % 7;
      }

      if (buttonClicks[2]) {
        buttonClicks[2] = false;
        if (pageNum != 0) return;
        displayChangeFlag = true;
        selectedOption = (selectedOption + 1) % 7;
      }

      if (buttonClicks[3]) {
        buttonClicks[3] = false;
        if (pageNum == 0 && selectedOption == 6) {
          if (requedSaving == false) return;
          requedSaving = false;
          displayChangeFlag = true;
          return;
        } else if (pageNum != 0) {
          pageNum = (pageNum - 1 + 2) % 2;
          displayChangeFlag = true;
          fillRectNeeded = true;
        }
      }
    }
  }

  void handleDisplay() {
    displayChangeFlag = false;
    if (requedSaving == true) savingSymbol(ST77XX_WHITE);
    else savingSymbol(ST77XX_BLACK);
    if (pageNum == 0) mainMenu();
    else if (pageNum == 1) showGraph();
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

  const int LEDPINS[6] = { 48, 49, 50, 51, 52, 53 };
#define WDT_KEY (0xA5)
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)
  SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

  SdFs sd;
  FsFile file;

public:

  LCD_Display display;
  SDI_12 sdi12;

  void watchdogSetup(void) {
    /*** watchdogDisable (); ***/
  }

  void init() {
    initializeWatchdog();

    for (int pin : LEDPINS) { pinMode(pin, OUTPUT); }
    sdi12.setUp();
    if (!sd.begin(SD_CONFIG)) {
      Serial.println("SD card initialization failed!");
      return;
    }
    if (!file.open("MR_Sensor_Data.txt", O_RDWR | O_CREAT)) {
      Serial.println(F("open failed"));
    }
    //file.rewind();
    file.println("Date Time Temperature Humidity Pressure GasResistance Altitude LightLevel");
    file.println("-------------------------------------------------------------------------");
    file.close();
    display.init();
    display.startUp();
    display.setLayOut();
  }

  void handleLogger() {

    resetWatchdog();
    // If user is not in menu; check for double button press

    if (!display.menuStarted) display.displayStart();

    // If user is in menu; check for a button press at every clock refresh (1 second timer)
    if (display.menuStarted && display.clockRefreshFlag) display.handleButtonClick();

    // Refresh the clock every 1 second
    if (display.clockRefreshFlag) display.refreshClock();

    // If the display needs to change; call the function
    if (display.displayChangeFlag) display.handleDisplay();

    // Check for serial input
    if (Serial1.available()) {
      String input = Serial1.readStringUntil('\n');
      char buf[input.length()];
      memset(buf, 0, sizeof(buf));

      for (int i = 1; i < input.length(); i++) {
        buf[i - 1] = input[i];
      }

      sdi12.runCommand(buf);
    }

    // If a continuous measurement is required
    if (sdi12.measureFlag) {
      sdi12.measureFlag = false;
      sdi12.handleMeasurement();
      // Puts data in sensorValues array
      for (int j = 0; j < 6; j++) {
        for (int i = 0; i < 20; i++) {
          display.sensorValues[j][i] = display.sensorValues[j][i + 1];
        }
        display.sensorValues[j][19] = sdi12.dataValue[j];
      }
      alertCheck();
      // If data is to be saved to the SD card
      if (display.requedSaving) {
        saveData(file, sdi12.dataValue, display.now);
      }
      if (display.pageNum == 1) display.displayChangeFlag = true;
    }
  }

  void alertCheck() {
    if (sdi12.dataValue[0] > 30) {
      digitalWrite(LEDPINS[0], HIGH);
    } else {
      digitalWrite(LEDPINS[0], LOW);
    }

    if (sdi12.dataValue[1] > 70) {
      digitalWrite(LEDPINS[1], HIGH);
    } else {
      digitalWrite(LEDPINS[1], LOW);
    }

    if (sdi12.dataValue[2] > 700) {
      digitalWrite(LEDPINS[2], HIGH);
    } else {
      digitalWrite(LEDPINS[2], LOW);
    }

    if (sdi12.dataValue[3] > 700) {
      digitalWrite(LEDPINS[3], HIGH);
    } else {
      digitalWrite(LEDPINS[3], LOW);
    }

    if (sdi12.dataValue[4] > 8000) {
      digitalWrite(LEDPINS[4], HIGH);
    } else {
      digitalWrite(LEDPINS[4], LOW);
    }

    if (sdi12.dataValue[5] > 50000) {
      digitalWrite(LEDPINS[5], HIGH);
    } else {
      digitalWrite(LEDPINS[5], LOW);
    }
  }

  void initializeWatchdog() {
    // Enable watchdog.
    WDT->WDT_MR = WDT_MR_WDD(0xFFF) | WDT_MR_WDFIEN |  //  Triggers an interrupt or WDT_MR_WDRSTEN to trigger a Reset
                  WDT_MR_WDV(256 * 1);                 // Watchdog triggers a reset or an interrupt after 1 seconds if underflow

    NVIC_EnableIRQ(WDT_IRQn);

    uint32_t status = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos /*8*/;  // Get status from the last Reset
    //Serial.print("RSTTYP = 0b");
    //Serial.println(status, BIN);  // Should be 0b010 after first watchdog reset
  }

  void resetWatchdog() {
    //Restart watchdog
    WDT->WDT_CR = WDT_CR_KEY(WDT_KEY)
                  | WDT_CR_WDRSTT;

    //Serial.println("Enter the main loop : Restart watchdog");
    GPBR->SYS_GPBR[0] += 1;
    Serial.print("GPBR = ");
    Serial.println(GPBR->SYS_GPBR[0]);
  }


  void saveData(FsFile file, float dataValue[6], DateTime now) {
    file.open("MR_Sensor_Data.txt", O_RDWR);
    file.seekEnd();
    file.print(now.timestamp(DateTime::TIMESTAMP_DATE));
    file.print(" ");
    file.print(now.timestamp(DateTime::TIMESTAMP_TIME));
    file.print(" ");
    for (int i = 0; i < 6; i++) {
      if (i != 5) {
        file.print(dataValue[i]);
        file.print(" ");
      } else {
        file.print(dataValue[i]);
      }
    }
    file.println();
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

void TC0_Handler(void) {
  TC_GetStatus(TC0, 0);
  logger.sdi12.measureFlag = true;
}

void TC3_Handler(void) {
  TC_GetStatus(TC1, 0);
  logger.display.clockRefreshFlag = true;
}

void WDT_Handler(void) {
  WDT->WDT_SR;  // Clear status register

  printf("in WDT\n");
}
