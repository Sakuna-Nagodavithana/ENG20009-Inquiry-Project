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

volatile int goBackNum = 0;
volatile int goDownNum = 0;
volatile int goUpNum = 0;
volatile int selectedOption = 0;
volatile int goFrontNum = 0;

void setup(void) {

  for(int pin:buttonPins){
    pinMode(pin,INPUT_PULLUP);
  }

  attachInterrupt(digitalPinToInterrupt(buttonPins[0]), goBack, FALLING); 
  attachInterrupt(digitalPinToInterrupt(buttonPins[1]), goDown, FALLING); 
  attachInterrupt(digitalPinToInterrupt(buttonPins[2]), goUp, FALLING); 
  attachInterrupt(digitalPinToInterrupt(buttonPins[3]), goFront, FALLING); 
  Serial.begin(9600);
  //Serial.print(F("Hello! ST77xx TFT Test"));
  
  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);
  rtc.begin();
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  setLayOut();

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("SD card initialization failed!");
    sd.initErrorHalt();
    while (1);
  }


  
}

void loop() {
  goBackNum = constrain(goBackNum,0,1);
  goUpNum = constrain(goUpNum,0,1);
  goDownNum = constrain(goDownNum,0,1);
  goFrontNum = constrain(goFrontNum,0,1);

  tft.setTextColor(ST77XX_BLUE);
  switch(selectedOption){
    case 0:{
      tft.setCursor(13, 22);
      tft.println("Sensor 1");
      break;
    }
    case 1:{
      tft.setCursor(13, 36);
      tft.println("Sensor 2");
      break;
    }
  }

  DateTime now = rtc.now();

  
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


  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(13, 22);
  tft.println("Sensor 1");

  tft.setCursor(13, 36);
  tft.println("Sensor 2");




  

}


void goBack(){
  //goBackNum++;
}

void goDown(){
  Serial.print("Before Minu : ");Serial.println(selectedOption);
  selectedOption = constrain(--selectedOption,0,1);
  Serial.print("After Minu : ");Serial.println(selectedOption);

}

void goUp(){
  Serial.print("Before Add : ");Serial.println(selectedOption);
  selectedOption = constrain(++selectedOption,0,1);
  Serial.print("After Add : ");Serial.println(selectedOption);
}

void goFront(){
  //goFront++;
}


void setLayOut(){
  
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

