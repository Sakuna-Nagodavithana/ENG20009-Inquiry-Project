#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "RTClib.h" 
#include "SdFat.h"

// SdFs sd; // SD card filesystem
// FsFile file;


// //^ configuration for FAT16/FAT32 and exFAT.

// // Chip select may be constant or RAM variable.
// const uint8_t SD_CS_PIN = A3;
// //
// // Pin numbers in templates must be constants.
// const uint8_t SOFT_MISO_PIN = 12;
// const uint8_t SOFT_MOSI_PIN = 11;
// const uint8_t SOFT_SCK_PIN  = 13;

// // SdFat software SPI template
// SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

// #define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)



static volatile bool buttonClicks[4] = {false, false, false, false};
static volatile bool anyButtonClick = false;



class LCD_Display {
  private:
    const int TFT_CS = 10;
    const int TFT_RST =  6 ;
    const int TFT_DC = 7; 

    const int TFT_SCLK = 13;   
    const int TFT_MOSI = 11;   

    //Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
    Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
    
  public:
    RTC_DS1307 rtc;
    const int buttonPins[4] = {2, 3, 4, 5};
    bool fillRectNeeded = true;

    unsigned long last_interrupt_time = 0;
    unsigned long delayTime = 500;

    volatile int selectedOption = 0;
    volatile int pageNum = 0;

    static void goBack(){
      
      buttonClicks[0] = true;
      anyButtonClick = true;
    }

    static void goDown(){
      
      buttonClicks[1] = true;
      anyButtonClick = true;
    }

    static void goUp(){
      buttonClicks[2] = true;
      anyButtonClick = true;
    }

    static void goFront(){
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
        tft.fillRect(0, 15, 120, 85, ST77XX_BLACK);
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
        tft.fillRect(0, 15, 120, 85, ST77XX_BLACK);
      }
      
      tft.drawCircle(50, 50, 10, ST77XX_WHITE);
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


};

LCD_Display display;
void setup() {
  
  display.setLayOut();

}

void loop() {

  
  DateTime now = display.rtc.now();
  display.displayTime(now);

  display.handleButtons();

}
