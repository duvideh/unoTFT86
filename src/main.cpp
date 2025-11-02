// Arduino MEGA board
// Adafruit 1.44" 128x128 TFT screen
// Voltage divider sourced from 12v socket for Battery Voltage
// used internal voltage reference ***CAUTION*** do not connect >1v to any analogRead pin!!!
// Switch for 86 VSC buttons - all off on startup - external switch to disable
// softwareSerial for reading data from Nano in glovebox - Expected message: <#,#,#>

#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <SoftwareSerial.h>
#include <SPI.h>
#include <FlickerFreePrint-master/FlickerFreePrint.h>
#include <eightySixFont20.h>
#include <eightySixFont15.h>
#include <eightySixFont10.h>
#include <bitmaps.h>
#include <INA226.h>
 
// TFT (use tft.### functions i.e. tft.println() )
 #define SD_CS          13 // SD card select pin
 #define TFT_DC         8 // TFT display/command pin
 #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
 #define TFT_CS        10 // TFT select pin 
 #define SPI_BUSY (!(SPSR & (1 << SPIF)))     // non-zero if SPI transmitter bus  //https://forums.adafruit.com/viewtopic.php?t=198726

 Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); 

//SD Card stuff 
 SdFat SD; // SD card filesystem
 Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys 
 Adafruit_Image       img;        // An image loaded into RAM
 int32_t              width  = 0, // BMP image dimensions
                      height = 0;
 
 //canvas
   //oil temp digits
      #define W  59
      #define H  28
   //coolant temp digits
      #define W2 42
      #define H2 20
   //voltage digits
      #define W3 45
      #define H3 20
    //coolant and voltage icons
      #define W4 21
      #define H4 20
    //86 logo
      #define W5 107
      #define H5 31
    //oil lamp
      #define W6 47
      #define H6 27
    //gauge
      #define W7 128
      #define H7 15
   GFXcanvas1 canvas(W, H);   //oil temp digits
   GFXcanvas1 canvas2(W2, H2); //coolant tem pdigits
   GFXcanvas1 canvas3(W3, H3); //voltage digits
   GFXcanvas1 canvas4(W4, H4); //coolant and voltage icons
   GFXcanvas1 canvas5(W5, H5); //86 logo
   GFXcanvas1 canvas6(W6, H6); //oil lamp
   GFXcanvas1 canvas7(W7, H7); //gauge

//variables to hold cursor coordinates
 int x = 0;
 int y = 0;

//colours
 uint16_t black = 0x0000;
 uint16_t white = 0xFFFF;
 uint16_t red = 0xF800;
 uint16_t limeGreen = 0xE7E0;
 uint16_t blue = 0x001F;
 uint16_t green = 0x07E0;
 uint16_t cyan = 0x07FF;
 uint16_t orange = 0xFD00;
 uint16_t lightRed = 0xFD76;
 uint16_t greenDark = 0x05AB;
 uint16_t blueDark = 0x3997;
 uint16_t color = white;
 uint16_t color2 = red;
 uint16_t greenDim = green;
 uint16_t blueDim = blue;
 uint16_t redDim = lightRed;

//Software Serial  https://forum.arduino.cc/t/serial-input-basics-updated/382007/3
 #define rxPin 11
 #define txPin 42
 SoftwareSerial softSerial (rxPin, txPin);   // Set up a new SoftwareSerial object
 const byte numChars = 32;        //32 bit serial buffer
 char receivedChars[numChars];
 char tempChars[numChars];        // temporary array for use when parsing  
 boolean newData = false;

// variables to hold the parsed data
 int oilTemp = 0;
 int coolantTemp = 1;
 int previousCoolantTemp = 0;
 int coolantTemporary = 0;
 bool coolantStart = 1;      //initial value to set previousCoolantTemp to first reading
 int voltage = 0;
 int disp1 = 0;
 int disp2 = 0;
 int disp3 = 0;
 
//for blanking screen over bigger/smaller numbers
 bool start = 0;
 bool change1 = 0;
 bool flash = 0; //enable/disable flashing of oil temp when threshold exceeded
 bool flashActivate = 0;
 int size = 0;

//INA226 voltage sensor
 float voltRead = 0.0;
 INA226 INA(0x40);  //I2C device identifier
 
//headlight dimming
 #define headlightSignal 31  //input from Nano
 #define Lite 12             //output to TFT backlight
 int headlights = 0;        
 bool headlightStatus = 0;
 
//VSC control 
 #define VSC_out 5
 
//max value reading and mode buttons
//#define max_pin 7 //button to display maximumn recorded value for each parameter
//int max_state = 0; // state of max button
//int modeButton = 0; // counter for the number of mode button presses
 
//millis to set delay between cycles of program
 unsigned long millis10 = 0;
 unsigned long millis50 = 0;
 unsigned long millis200 = 0;

//flickerFreePrint displays
  FlickerFreePrint<Adafruit_ST7735> Data1(&tft, white, black);
  FlickerFreePrint<Adafruit_ST7735> Data2(&tft, redDim, black);

//  __      ______ _____ _____
//  \ \    / / __ \_   _|  __ \ "/"
//   \ \  / / |  | || | | |  | |
//    \ \/ /| |  | || | | |  | |
//     \  / | |__| || |_| |__| |
//      \/   \____/_____|_____/

//============

//   __          _   _____                     ____  _ _                         
//  / _|        | | |  __ \                   |  _ \(_) |                        
// | |_ __ _ ___| |_| |  | |_ __ __ ___      _| |_) |_| |_ _ __ ___   __ _ _ __  
// |  _/ _` / __| __| |  | | '__/ _` \ \ /\ / /  _ <| | __| '_ ` _ \ / _` | '_ \ 
// | || (_| \__ \ |_| |__| | | | (_| |\ V  V /| |_) | | |_| | | | | | (_| | |_) |
// |_| \__,_|___/\__|_____/|_|  \__,_| \_/\_/ |____/|_|\__|_| |_| |_|\__,_| .__/ 
//                                                                         |_|    
//                                                                             
void fastDrawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg) //https://forums.adafruit.com/viewtopic.php?t=198726
{
  //x += 40;                                   // mysterious X offset
  //y += 53;                                   // mysterious Y offset
  while SPI_BUSY;  digitalWrite(TFT_CS, 0);  // indicate "transfer"
  while SPI_BUSY;  digitalWrite(TFT_DC, 0);  // indicate "command"
  while SPI_BUSY;  SPDR = 0x2A;              // send column span command
  while SPI_BUSY;  digitalWrite(TFT_DC, 1);  // indicate "data"
  while SPI_BUSY;  SPDR = (x)     >> 8;      // send Xmin
  while SPI_BUSY;  SPDR = (x)     >> 0;
  while SPI_BUSY;  SPDR = (x+w-1) >> 8;      // send Xmax
  while SPI_BUSY;  SPDR = (x+w-1) >> 0;
  while SPI_BUSY;  digitalWrite(TFT_DC, 0);  // indicate "command"
  while SPI_BUSY;  SPDR = 0x2B;              // send row span command
  while SPI_BUSY;  digitalWrite(TFT_DC, 1);  // indicate "data"
  while SPI_BUSY;  SPDR = (y)     >> 8;      // send Ymin
  while SPI_BUSY;  SPDR = (y)     >> 0;
  while SPI_BUSY;  SPDR = (y+h-1) >> 8;      // send Ymax
  while SPI_BUSY;  SPDR = (y+h-1) >> 0;
  while SPI_BUSY;  digitalWrite(TFT_DC, 0);  // indicate "command"
  while SPI_BUSY;  SPDR = 0x2C;              // send write command
  while SPI_BUSY;  digitalWrite(TFT_DC, 1);  // indicate "data"

  int16_t byteWidth = (w + 7) >> 3;          // bitmap width in bytes
  int8_t bits8 = 0;
  for (int16_t j = 0; j < h; j++)
    for (int16_t i = 0; i < w; i++)
    {
      bits8 = i & 7 ? bits8 << 1 : bitmap[j * byteWidth + (i >> 3)];  // fetch next pixel
      uint16_t c = bits8 < 0 ? color : bg;   // select color
      while SPI_BUSY;  SPDR = c >> 8;        // send color
      while SPI_BUSY;  SPDR = c >> 0;
    }
  digitalWrite(TFT_CS, 1);                   // indicate "idle"
}


//      _ _                               
//   __| (_)_ __ ___  _ __ ___   ___ _ __ 
//  / _` | | '_ ` _ \| '_ ` _ \ / _ \ '__|
// | (_| | | | | | | | | | | | |  __/ |   
//  \__,_|_|_| |_| |_|_| |_| |_|\___|_|   
//
void dimmer() {  
  if (headlights == 1) {
    color = orange;
    color2 = red;
    greenDim = greenDark;
    blueDim = blueDark;
    redDim = red;
    }
  else {
    color = white;
    color2 = white;
    greenDim = limeGreen;
    blueDim = cyan;
    redDim = lightRed;
   }
  //bitmaps
  //fastDrawBitmap(22, 74, canvas2.getBuffer(), W2, H2, color2, black);
    canvas4.fillScreen(black);
    canvas5.fillScreen(black);
    canvas6.fillScreen(black);
    canvas7.fillScreen(black);
    canvas4.drawBitmap(0, 0, coolant, 20, 20, color2);
    canvas5.drawBitmap(0, 0, logo, 107, 31, color2);
    canvas6.drawBitmap(0, 0, oil_lamp, 47, 27, color2);
    canvas7.drawBitmap(0, 0, gauge, 128, 15, color2);
    fastDrawBitmap(2, 32, canvas7.getBuffer(), 128, 15, color, black);
    fastDrawBitmap(12, 99, canvas5.getBuffer(), 107, 31, color, black);
    fastDrawBitmap(2, 3, canvas6.getBuffer(), 47, 27, color, black);
    fastDrawBitmap(2, 73, canvas4.getBuffer(), 20, 20, color, black);
    canvas4.fillScreen(black);
    canvas4.drawBitmap(0, 0, battSmall, 20, 20, color2);
    fastDrawBitmap(65, 73, canvas4.getBuffer(), 20, 20, color, black);
    //tft.drawRGBBitmap(10, 95,epd_bitmap_, 107, 32);  //86_color
  //oil temp gauge horizontal bar digits
    tft.setTextColor(color);
    tft.setFont(&eightySixFont10);      
    tft.setTextSize(1);
    tft.setCursor(0,65);
    tft.print(0);
    tft.setCursor(53,65);
    tft.print(85);
    tft.setCursor(99,65);
    tft.print(170);
    tft.drawFastHLine(0,66,128,color);
  // deg. C
    tft.setTextColor(color2);  
    tft.setFont(&eightySixFont20);
    tft.setCursor(108,28);
    tft.print("C");
}

//                       __    __ _ _   _     __ _             _     __          _                   _                 
//  _ __ ___  _____   __/ / /\ \ (_) |_| |__ / _\ |_ __ _ _ __| |_  /__\ __   __| | /\/\   __ _ _ __| | _____ _ __ ___ 
// | '__/ _ \/ __\ \ / /\ \/  \/ / | __| '_ \\ \| __/ _` | '__| __|/_\| '_ \ / _` |/    \ / _` | '__| |/ / _ \ '__/ __|
// | | |  __/ (__ \ V /  \  /\  /| | |_| | | |\ \ || (_| | |  | |_//__| | | | (_| / /\/\ \ (_| | |  |   <  __/ |  \__ \ "/"
// |_|  \___|\___| \_/    \/  \/ |_|\__|_| |_\__/\__\__,_|_|   \__\__/|_| |_|\__,_\/    \/\__,_|_|  |_|\_\___|_|  |___/
                                                                                                                    
void recvWithStartEndMarkers() {
   static boolean recvInProgress = false;
   static byte ndx = 0;
   char startMarker = '<';
   char endMarker = '>';
   char rc;

  while (softSerial.available() > 0 && newData == false) {
   rc = softSerial.read();
   if (recvInProgress == true) {
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
       }
      }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      recvInProgress = false;
      ndx = 0;
      newData = true;
      } 
    }
  else if (rc == startMarker) {
      recvInProgress = true;
     } 
   }
 }

//                               ___      _        
//  _ __   __ _ _ __ ___  ___   /   \__ _| |_ __ _ 
// | '_ \ / _` | '__/ __|/ _ \ / /\ / _` | __/ _` |
// | |_) | (_| | |  \__ \  __// /_// (_| | || (_| |
// | .__/ \__,_|_|  |___/\___/___,' \__,_|\__\__,_|
// |_|                                             

void parseData() {      // split the data to send into its parts

 char * strtokIndx; // this is used by strtok() as an index

 strtokIndx = strtok(tempChars, ","); //NULL is after first delimiter, before first delimiter use tempChars
 oilTemp = atoi(strtokIndx);     // atoi = conver string to integer

 //2nd digit in serial sequence
 strtokIndx = strtok(NULL, ",");
 coolantTemp = atoi(strtokIndx);

 // //3rd digit etc..
 // strtokIndx = strtok(NULL, ",");
 // voltage = atoi(strtokIndx);
}


//    _____    __  _             _ _       
//    \_   \/\ \ \/_\__   _____ | | |_ ___ 
//     / /\/  \/ //_\\ \ / / _ \| | __/ __|
///  \/ /_/ /\  /  _  \ V / (_) | | |_\__ \ "/"
//  \____/\_\ \/\_/ \_/\_/ \___/|_|\__|___/

void INAvolts() {
  voltRead = INA.getBusVoltage();
}



//   _____ ______ _______ _    _ _____
//  / ____|  ____|__   __| |  | |  __ \ "/"
// | (___ | |__     | |  | |  | | |__) |
//  \___ \|  __|    | |  | |  | |  ___/
//  ____) | |____   | |  | |__| | |
// |_____/|______|  |_|   \____/|_|

void setup()
{
  Serial.begin(9600);
  softSerial.begin(9600);

  //INA226 voltage sensor setup
    Wire.begin();              
    if (!INA.begin())
      {
        Serial.println("No connection to INA226. Please fix.");
      }     
    INA.setMaxCurrentShunt(1,0.002);

  //headlights I/O
    pinMode(Lite,OUTPUT);
    digitalWrite(Lite,LOW);
    pinMode(headlightSignal,INPUT);

  //VSC
    pinMode(VSC_out, OUTPUT); // VSC output

  //establish pins 2 and 3 as interrupts, mode increase button
  //  pinMode(max_pin, INPUT); // max button
  //  attachInterrupt(digitalPinToInterrupt(2), mode_counter_increase, RISING);  // modeButton = 1
  //  attachInterrupt(digitalPinToInterrupt(3), mode_counter_decrease, RISING);  // modeButton = 0

  // TFT
    tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
    //SD Card stuff
      ImageReturnCode stat; // Status from image-reading functions
        // The Adafruit_ImageReader constructor call (above, before setup())
        // accepts an uninitialized SdFat or FatVolume object. This MUST
        // BE INITIALIZED before using any of the image reader functions!
      Serial.print(F("Initializing filesystem..."));
      if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
        Serial.println(F("SD begin() failed"));
        for(;;); // Fatal error, do not continue
      }
      Serial.println(F("OK!")); 
      Serial.print(F("Loading 86x128.bmp to screen..."));
      stat = reader.drawBMP("/86x128.bmp", tft, 0, 0);
      reader.printStatus(stat);   // How'd we do?     
      analogWrite(Lite,255); //turn on backlight - after 86x128.bmp first displayed so white screen doesn't appear
  
  delay(1000);
  digitalWrite(VSC_out, HIGH); //'hold down' VSC button
  delay(3200);
  digitalWrite(VSC_out, LOW); //'release' VSC button
  
  //initial writing of characters, fonts to canvases
    tft.fillScreen(black);
    canvas.fillScreen(black);
    canvas.setTextWrap(false);
    canvas.setFont(&eightySixFont20);
    canvas.setTextSize(1);
    canvas2.fillScreen(black);
    canvas2.setTextWrap(false);
    canvas2.setFont(&eightySixFont15);
    canvas2.setTextSize(1);
    canvas3.fillScreen(black);
    canvas3.setTextWrap(false);
    canvas3.setTextSize(1);
  
  //check if headlights on, set dimming accordingly
    dimmer();

  //establishment of delay-less timing protocols
    millis10 = millis();
    millis200 = millis();
}

//   _      ____   ____  _____
//  | |    / __ \ / __ \|  __ \  "/"
//  | |   | |  | | |  | | |__) |
//  | |   | |  | | |  | |  ___/
//  | |___| |__| | |__| | |
//  |______\____/ \____/|_|

void loop() 
{
 //max_state = digitalRead(max_pin); //to setup in future

 if ( millis() >= millis10 + 10) {
    headlights = digitalRead(headlightSignal);
    //Serial.println(headlights);
      if (headlights != headlightStatus) {
        dimmer();
      }

   if (headlights == 1) {                     
      analogWrite(Lite,50);
      headlightStatus = 1;
    }
    else {
      analogWrite(Lite,255);
      headlightStatus = 0;
    }

    millis10 = millis();
  }
 
 if ( millis() >= millis200 + 200 ) {
  //get voltage from INA226
    INAvolts();
    //testing
    // tft.setCursor(0,35);
    // tft.setTextColor(green);
    // tft.print(voltRead,4);  
  
  //collect data from softSerial
    recvWithStartEndMarkers();
    if (newData == true) {
      //nudat = 1;  //debugging
      //Serial.println(nudat);
      strcpy(tempChars, receivedChars);
        // this temporary copy is necessary to protect the original data
        //   because strtok() used in parseData() replaces the commas with \0
      parseData();
      newData = false;
    }
        
   //Print oil temp gauge - horizontal bar
    if (oilTemp < 85) {
      flashActivate = 0;
      tft.fillRect(2,48,2,4,blueDim);
      tft.drawRect(0,46,(oilTemp*0.7529)+1,8,color);
      tft.drawRect(1,47,(oilTemp*0.7529)-1,6,color);
      tft.fillRect(2,48,(oilTemp*0.7529)-3,4,blueDim);
      tft.fillRect((oilTemp*0.7529)+1, 46, (128-(oilTemp*0.7529)), 8, black);
    }
    else if (oilTemp >= 85 && oilTemp <=129) {
      flashActivate = 0;
      tft.fillRect(2,48,2,4,greenDim);
      tft.drawRect(0,46,(oilTemp*0.7529)+1,8,color);
      tft.drawRect(1,47,(oilTemp*0.7529)-1,6,color);
      tft.fillRect(2,48,(oilTemp*0.7529)-3,4,greenDim);
      tft.fillRect((oilTemp*0.7529)+1, 46, (128-(oilTemp*0.7529)), 8, black);
    }
    else if (oilTemp >= 130) {
      flashActivate = 1;
      tft.fillRect(2,48,2,4,redDim);
      tft.drawRect(0,46,(oilTemp*0.7529)+1,8,color);
      tft.drawRect(1,47,(oilTemp*0.7529)-1,6,color);
      tft.fillRect(2,48,(oilTemp*0.7529)-3,4,redDim);
      tft.fillRect((oilTemp*0.7529)+1, 46, (128-(oilTemp*0.7529)), 8, black);
    }

    //warning flashing
      if (millis() >= millis50 + 100) {
        flash = !flash;
        millis50 = millis();
      }

    //Oil temp
      canvas.setTextColor(color2,black);
      static byte value = 0;  ///unused value????
      canvas.fillScreen(black); //erase canvas
      //establish cursor position based on number of digits 1, 2 or 3 (i.e 9, 19, 119) - basically make text right-aligned instead of default left-aligned
        if (oilTemp < 10) {
          if (size != 1) {
          }
          size = 1;
        }
        if (oilTemp >= 10 && oilTemp < 100) {
          if (size != 2) {
          }
          size = 2;
        }
        if (oilTemp >= 100) {
          size = 3;
        }
      //print text to screen
        if (size == 1 ) { 
          //canvas.drawBitmap(2, 0, oilTemp_lamp, 47, 27, color);
          //numbers ending in 1, shift to the right
          if ((oilTemp % 10) == 1) {
            canvas.setCursor(46,28);//96,28);
          }
          else {
            canvas.setCursor(39,28);//89,28);
          }
          canvas.print(oilTemp);
          fastDrawBitmap(50, 2, canvas.getBuffer(), W, H, color2, black);
        }
        if (size == 2) {
          //canvas.drawBitmap(2, 0, oilTemp_lamp, 47, 27, color);
          if (oilTemp < 20) {            //compensate for reduced width of "1" digit
            canvas.setCursor(26,28);
          }
          else {
            canvas.setCursor(19,28);
          }
          if ((oilTemp % 10) == 1) {
            canvas.setCursor(canvas.getCursorX()+7,28);
          }
          canvas.print(oilTemp);
          fastDrawBitmap(50, 2, canvas.getBuffer(), W, H, color2, black);
        }
        if (size == 3 ) { 
          //canvas.drawBitmap(2, 0, oilTemp_lamp, 47, 27, color);
          if (oilTemp >=110 && oilTemp <=119) {
            canvas.setCursor(13,28);
          }
          else {
            canvas.setCursor(6,28);
          }
          if ((oilTemp % 10) == 1) {
            canvas.setCursor(canvas.getCursorX()+7,28);
          }
          if (flashActivate == 1) {
            if (flash == 1) {
              canvas.fillRect(63,28,60,20,black);
            }
            else {
              canvas.print(oilTemp);
            }
          }
          else {
            canvas.print(oilTemp);
          }
          fastDrawBitmap(50, 2, canvas.getBuffer(), W, H, color2, black);
          }

    //coolant temp
      if (coolantStart == 1) {
        previousCoolantTemp = coolantTemp;
        coolantStart = 0;
      }
      // if (coolantTemp = 0) {                // doesnt't work idk
      //   coolantTemp = previousCoolantTemp;
      // }
      if (coolantTemp + previousCoolantTemp != previousCoolantTemp) {
      canvas2.fillScreen(black);
      canvas2.setTextColor(color2);  //coolantTemp
      if (coolantTemp <=9) {
        canvas2.setCursor(2,19);
        canvas2.print(coolantTemp);
      }
      if (coolantTemp >= 10 && coolantTemp <= 19) {     //compensate for width of "1" digit
        canvas2.setCursor(0,19);
        canvas2.print(coolantTemp);
      }
      if (coolantTemp >= 20 && coolantTemp <=94) {
        canvas2.setCursor(2,19);
        canvas2.print(coolantTemp);
      }
      if (coolantTemp >= 95) {
        //cursor
          if (coolantTemp >= 100) {
            canvas2.setCursor(0,19);
          }
          else {
            canvas2.setCursor(2,19);
          }
        //flashing
          if (flash == 1) {
            canvas2.fillRect(20,0,60,20,black);
          }
          else {
            canvas2.print(coolantTemp);
          } 
      }
      fastDrawBitmap(22, 74, canvas2.getBuffer(), W2, H2, color2, black);
      previousCoolantTemp = coolantTemp;
      }
    
    //battery voltage
      // Shift the decimal point right two digits and round off to an integer
        int voltage = (voltRead/*batAvg*/ * 100.0) + 0.5;
      // Extract each digit with the 'modulo' operator (%)
        char tensDigit = '0' + ((voltage / 1000) % 10);
        char onesDigit = '0' + ((voltage / 100) % 10);
        char tenthsDigit =  '0' + (( voltage / 10) % 10);
        //char hundredsDigit =  '0' + (voltage % 10);
      //write to canvas
      canvas3.fillScreen(black);
      canvas3.setFont(&eightySixFont15);
      canvas3.setTextColor(color2);
      if (voltRead/*batAvg*/ < 1.0) {
        canvas3.setCursor(2,19);
      }
      else if (voltRead/*batAvg*/ >= 1.0 && voltRead/*batAvg*/ < 2.0) {
        canvas3.setCursor(0,19);
      }
      else if(voltRead/*batAvg*/ >= 2.0 && voltRead/*batAvg*/ < 10.0) {
        canvas3.setCursor(2,19);
      }
      else if (voltRead/*batAvg*/ >= 10.0) { 
        canvas3.setCursor(0,19);
        canvas3.print(tensDigit);
      }
      canvas3.print(onesDigit);
      canvas3.setCursor((canvas3.getCursorX()), ((canvas3.getCursorY())+1));
      canvas3.print(".");
      canvas3.setCursor((canvas3.getCursorX()), ((canvas3.getCursorY())-1));
      canvas3.setFont(&eightySixFont10);
      canvas3.print(tenthsDigit);
      fastDrawBitmap(86,74, canvas3.getBuffer(), W3, H3, color2, black);
    
  millis200 = millis();
  } 
}
