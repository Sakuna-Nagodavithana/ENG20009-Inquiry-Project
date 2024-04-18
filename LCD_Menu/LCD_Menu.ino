#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "RTClib.h" 
#include "SdFat.h"

SdFs sd; // SD card filesystem
FsFile file;


//^ configuration for FAT16/FAT32 and exFAT.

// Chip select may be constant or RAM variable.
const uint8_t SD_CS_PIN = A3;
//
// Pin numbers in templates must be constants.
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

// SdFat software SPI template
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)


//Use Male/Female jumper wire to connect pin 1 of J9 to pin 21, and pin 3 to pin 20 on Arduino DUE
//Arduino Zero, add shunt as normal to J9

RTC_DS1307 rtc;


#define TFT_CS    10
#define TFT_RST   6 
#define TFT_DC    7 

#define TFT_SCLK 13   
#define TFT_MOSI 11   

//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

const int buttonPins[] = {2,3,4,5};
volatile bool buttonClicks[] = {false,false,false,false}; 
volatile bool anyButtonClick = false;
bool fillRectNeeded = true;

static unsigned long last_interrupt_time = 0;
unsigned long delayTime = 500;

volatile int selectedOption = 0;
volatile int pageNum = 0;

void setup() {

  setLayOut();

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("SD card initialization failed!");
    sd.initErrorHalt();
    while (1);
  }


  
}

void loop() {

  

  DateTime now = rtc.now();
  displayTime(now);

  handleButtons();

  



 




  

}


void goBack(){
  
  buttonClicks[0] = true;
  anyButtonClick = true;
}

void goDown(){
  
  buttonClicks[1] = true;
  anyButtonClick = true;
}

void goUp(){
  buttonClicks[2] = true;
  anyButtonClick = true;
}

void goFront(){
  buttonClicks[3] = true;
  anyButtonClick = true;
}


void setLayOut(){
    for(int pin:buttonPins){
    pinMode(pin,INPUT_PULLUP);
  }

  attachInterrupt(digitalPinToInterrupt(buttonPins[0]), goBack, RISING); 
  attachInterrupt(digitalPinToInterrupt(buttonPins[1]), goDown, RISING); 
  attachInterrupt(digitalPinToInterrupt(buttonPins[2]), goUp, RISING); 
  attachInterrupt(digitalPinToInterrupt(buttonPins[3]), goFront, RISING); 
  Serial.begin(9600);
  //Serial.print(F("Hello! ST77xx TFT Test"));
  
  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);
  rtc.begin();
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

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
  tft.fillTriangle(97, 115, 102, 110,107 ,115, ST77XX_BLACK);
  tft.fillTriangle(99, 115, 102, 112, 105, 115, ST77XX_GREEN);
  tft.fillRect(102.75, 114, 1.5, 7, ST77XX_BLACK);
}

void displayTime(DateTime now){
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


void mainMenu(){
  if(fillRectNeeded){
    tft.fillRect(0, 15, 160, 85, ST77XX_BLACK);
  }
  tft.setTextColor(ST77XX_BLUE);
  switch(selectedOption){
    case 0:{
      tft.setCursor(15, 22);
      tft.println("Temperature");
      break;
    }
    case 1:{
      tft.setCursor(15, 34);
      tft.println("Humidity");
      break;
    }
    case 2:{
      tft.setCursor(15, 46);
      tft.println("Humidity");
      break;
    }
    case 3:{
      tft.setCursor(15, 58);
      tft.println("Gas Resistance");
      break;
    }
    case 4:{
      tft.setCursor(15, 70);
      tft.println("Altutude");
      break;
    }
    case 5:{
      tft.setCursor(15, 82);
      tft.println("Light Level");
      break;
    }

  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 22);
  tft.println("Temperature");


  tft.setCursor(15, 34);
  tft.println("Humidity");


  tft.setCursor(15, 46);
  tft.println("Humidity");


  tft.setCursor(15, 58);
  tft.println("Gas Resistance");


  tft.setCursor(15, 70);
  tft.println("Altutude");


  tft.setCursor(15, 82);
  tft.println("Light Level");
  fillRectNeeded = false;
}

void showGraph(){
  //setLayOut();
  if(fillRectNeeded){
    tft.fillRect(0, 15, 160, 85, ST77XX_BLACK);
  }
  
  tft.drawLine(21.64, 25, 21.64, 92.52, ST77XX_WHITE);
  tft.drawLine(21.64, 25, 19, 30.46, ST77XX_WHITE);
  tft.drawLine(21.64, 25, 24.27, 30.46, ST77XX_WHITE);
  tft.drawLine(21.64, 92.52, 139, 92.52, ST77XX_WHITE);
  tft.drawLine(139, 92.52, 132.41, 88.06, ST77XX_WHITE);
  tft.drawLine(139, 92.52, 132.41, 96, ST77XX_WHITE);

  tft.setCursor(15, 14);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Y axis");
  
  
  tft.setCursor(68, 92);
  tft.print("X axis");
  int y[] = {1,2,3,4,5};
  tft.drawLine(25, 85.5 , 45, 85.5 - y[0], ST77XX_WHITE);
  tft.drawLine(45 , 85.5 - y[0], 65, 85.5 - y[1], ST77XX_WHITE);
  tft.drawLine(65, 85.5 - y[1], 85, 85.5 - y[2], ST77XX_WHITE);
  tft.drawLine(85, 85.5 - y[2], 105, 85.5 - y[3], ST77XX_WHITE);
  tft.drawLine(105 , 85.5 - y[3], 125, 85.5 - y[4], ST77XX_WHITE);
  fillRectNeeded = false;
}

void handleButtons(){
  if(millis() - last_interrupt_time > delayTime && anyButtonClick){
    
    if (buttonClicks[0]) {
      pageNum = (pageNum + 1) % 2;
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
      pageNum = (pageNum - 1 + 2) % 2;
      buttonClicks[3] = false;
      fillRectNeeded = true;
    }
    Serial.print("Slected Option = ");Serial.println(selectedOption);
    last_interrupt_time = millis();
    anyButtonClick = false;
  }


  if(pageNum == 0){
    mainMenu();
    Serial.println("Working");
  } else if (pageNum == 1){
    showGraph();
  }
  Serial.print("PageNO : ");Serial.println(pageNum);
  
}

