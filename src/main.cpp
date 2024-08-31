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
 
 // TFT (use tft.### functions i.e. tft.println() )
 #define SD_CS          13 // SD card select pin
 #define TFT_DC         8 // TFT display/command pin
 #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
 #define TFT_CS        10 // TFT select pin 
 
 Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); 

 //SD Card stuff 
SdFat SD; // SD card filesystem
Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys 

 Adafruit_Image       img;        // An image loaded into RAM
 int32_t              width  = 0, // BMP image dimensions
                      height = 0;
 
 //variables to hold cursor coordinates
 int x = 0;
 int y = 0;

 //colours
 uint16_t black = 0x0000;
 uint16_t white = 0xFFFF;
 uint16_t red = 0xF800;
 uint16_t light_red = 0xE7E0;
 uint16_t blue = 0x001F;
 uint16_t yellow = 0xFFE0;
 uint16_t green = 0x07E0;
 uint16_t cyan = 0x07FF;
 uint16_t magenta = 0xF81F;
 uint16_t orange = 0xfd00;
 uint16_t greenDark = 0x05ab;
 uint16_t blueDark = 0x3997;
 uint16_t color = white;
 uint16_t color2 = red;

//Software Serial  https://forum.arduino.cc/t/serial-input-basics-updated/382007/3
 #define rxPin 11
 #define txPin 42
 SoftwareSerial softSerial (rxPin, txPin);   // Set up a new SoftwareSerial object
 const byte numChars = 32;        //32 bit serial buffer
 char receivedChars[numChars];
 char tempChars[numChars];        // temporary array for use when parsing  

 // variables to hold the parsed data
 int int1 = 0;
 int int2 = 0;
 int int3 = 0;
 int disp1 = 0;
 int disp2 = 0;
 int disp3 = 0;

 //variables to clear screen when bigger number
 boolean newData = false;
 
 //for blanking screen over bigger/smaller numbers
 bool start = 0;
 bool big1 = 0;
 bool change1 = 0;
 bool flash = 0; //enable/disable flashing of oil temp when threshold exceeded
 bool flashActivate = 0;
 int size = 0;

//battery voltage
 #define batAnalogIn A0
 float batSensor = 0;
 float batVoltage = 0;
 float batLast = 0;
 float batAvg = 0;
 //***calibrate here*** 
 float aref = 1.063; //change this to the actual Aref voltage of ---YOUR--- Arduino 
 float r1 = 98.5; //insert first resistor value
 float r2 = 6.73; //insert second resistor value
 
//headlight dimming
 #define headlightSignal 31  //input from Nano
 #define Lite 12             //output to TFT backlight
 int headlights = 0;
 bool headlightStatus = 0;
 
//VSC control 
 #define VSC_out 5 //pin to control state of VSC 
 
//max value reading and mode buttons
//#define max_pin 7 //button to display maximumn recorded value for each parameter
//int max_state = 0; // state of max button
//int modeButton = 0; // counter for the number of mode button presses
 
 //debugging
 //bool nudat = 0;
 //int increase = 0;
 
 //millis to set delay between cycles of program
 unsigned long millis10 = 0;
 unsigned long millis50 = 0;
 unsigned long millis200 = 0;

static const unsigned char PROGMEM oil_lamp[] = {
  // 'Oil Temp', 47x32px
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, 
  0x3e, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x3f, 0xc0, 0x3f, 0x80, 0x00, 0x00, 0x3f, 0xf0, 0x38, 0x00, 
  0x00, 0xf8, 0x7f, 0xf0, 0x38, 0x00, 0x0f, 0xfc, 0x79, 0xf0, 0x38, 0x00, 0x3f, 0xfe, 0x78, 0x3c, 
  0x3f, 0x81, 0xff, 0x9c, 0xfc, 0x3e, 0x3f, 0x87, 0xff, 0x00, 0xff, 0xbe, 0x38, 0x1f, 0xfe, 0x00, 
  0x3f, 0xfc, 0x38, 0x3f, 0x9e, 0x08, 0x0f, 0xf8, 0x38, 0x7c, 0x1c, 0x1c, 0x03, 0xf8, 0x7c, 0x70, 
  0x38, 0x1c, 0x00, 0x78, 0xfe, 0x00, 0x70, 0x1c, 0x00, 0x38, 0xfe, 0x00, 0xe0, 0x1c, 0x00, 0x38, 
  0x7c, 0x01, 0xe0, 0x3e, 0x00, 0x38, 0x38, 0x03, 0xc0, 0x3e, 0x00, 0x38, 0x00, 0x07, 0x80, 0x3e, 
  0x00, 0x38, 0x00, 0x07, 0x00, 0x3e, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x1c, 0x00, 0x3f, 0xff, 0xfe, 
  0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

// 'gauge', 128x15px
static const unsigned char PROGMEM gauge[] = {
  0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 
	0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfc, 0x00, 0x78, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0xe0, 0x03, 0xc0, 0x0f, 0x80, 0x3f, 0x00, 0x7f, 
	0xfc, 0x00, 0x78, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0xe0, 0x03, 0xc0, 0x0f, 0x80, 0x3f, 0x00, 0x7f, 
	0xfc, 0x00, 0x78, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0xe0, 0x03, 0xc0, 0x0f, 0xff, 0xff, 0xff, 0xff, 
	0xfc, 0x00, 0x78, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0xe0, 0x03, 0xc0, 0x0f, 0xff, 0xff, 0xff, 0xff, 
	0xfc, 0x00, 0x78, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0xe0, 0x03, 0xc0, 0x0f, 0x00, 0x1e, 0x00, 0x3f, 
	0xfc, 0x00, 0x78, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0xe0, 0x03, 0xc0, 0x0f, 0x00, 0x1e, 0x00, 0x3f, 
	0xfc, 0x00, 0x30, 0x00, 0xc0, 0x01, 0x80, 0x07, 0xe0, 0x01, 0x80, 0x06, 0x00, 0x0c, 0x00, 0x3f, 
	0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 
	0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 
	0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 
	0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c
  };
//battery
static const unsigned char battSmall [] PROGMEM = {
  0x30, 0x00, 0x00, 0x30, 0x00, 0x00, 0xfc, 0x03, 0xf0, 0xfc, 0x03, 0xf0, 0x30, 0x00, 0x00, 0x30, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x30, 0x00, 0xc0, 0xff, 0xff, 
  0xf0, 0xff, 0xff, 0xf0, 0xc0, 0x00, 0x30, 0xc0, 0x00, 0x30, 0xc0, 0x00, 0x30, 0xc0, 0x00, 0x30, 
  0xc0, 0x00, 0x30, 0xc0, 0x00, 0x30, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xf0
  };
// 'Coolant small', 26x30px
static const unsigned char coolant [] PROGMEM = {
  0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x60, 0x00, 0x00, 
  0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x60, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x60, 
  0x00, 0x78, 0x61, 0xe0, 0xfc, 0xf3, 0xf0, 0xce, 0xf7, 0x30, 0x00, 0xf0, 0x00, 0x7c, 0x63, 0xe0, 
  0xfe, 0x07, 0xf0, 0xc7, 0x0e, 0x30, 0x03, 0xfc, 0x00, 0x01, 0xf8, 0x00
  };
//86 logo
static const unsigned char logo [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x07, 0x80, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 
	0x7f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdb, 0xff, 0xe0, 0x00, 0x00, 0x38, 0x1f, 0x83, 
	0x80, 0x00, 0x00, 0xff, 0xfb, 0x60, 0xdb, 0xff, 0xe0, 0x00, 0x00, 0x70, 0x31, 0x81, 0xc0, 0x00, 
	0x00, 0xff, 0xfb, 0x60, 0xdb, 0xff, 0xe0, 0x00, 0x00, 0x60, 0x33, 0x00, 0xc0, 0x00, 0x00, 0xff, 
	0xfb, 0x60, 0xdb, 0xff, 0xf8, 0x00, 0x00, 0xe0, 0x62, 0x00, 0x60, 0x00, 0x00, 0xff, 0xfb, 0x60, 
	0xdb, 0xff, 0xf8, 0x00, 0x01, 0xc0, 0x46, 0x00, 0x70, 0x00, 0x03, 0xff, 0xfb, 0x60, 0xdb, 0xff, 
	0xd8, 0x00, 0x01, 0x80, 0xfc, 0x7f, 0x30, 0x00, 0x03, 0x7f, 0xfb, 0x60, 0xdb, 0xff, 0xd8, 0x00, 
	0x01, 0x80, 0x78, 0x3f, 0x30, 0x00, 0x03, 0x7f, 0xfb, 0x60, 0xdb, 0xff, 0xd8, 0x00, 0x03, 0x80, 
	0x00, 0x23, 0x38, 0x00, 0x03, 0x7f, 0xfb, 0x60, 0xdb, 0xc3, 0xff, 0xff, 0xff, 0x00, 0x00, 0x63, 
	0xff, 0xff, 0xff, 0xf8, 0x7b, 0x60, 0xdb, 0x81, 0xbf, 0xff, 0xff, 0x07, 0xf0, 0x43, 0xff, 0xff, 
	0xff, 0xb0, 0x3b, 0x60, 0xdb, 0x81, 0xbf, 0xff, 0xff, 0x06, 0x20, 0xc0, 0x1f, 0xff, 0xff, 0xb0, 
	0x3b, 0x60, 0xdb, 0x81, 0xbf, 0xff, 0xff, 0x06, 0x60, 0xff, 0xff, 0xff, 0xff, 0xb0, 0x3b, 0x60, 
	0xdb, 0x81, 0xbf, 0xff, 0xff, 0x0c, 0x40, 0x00, 0x3f, 0xff, 0xff, 0xb0, 0x3b, 0x60, 0xdb, 0xc3, 
	0xff, 0xff, 0xff, 0x08, 0xc0, 0x00, 0x3f, 0xff, 0xff, 0xf8, 0x7b, 0x60, 0xdb, 0xff, 0xd8, 0x00, 
	0x03, 0x98, 0x80, 0x00, 0x38, 0x00, 0x03, 0x7f, 0xfb, 0x60, 0xdb, 0xff, 0xd8, 0x00, 0x01, 0x9f, 
	0x87, 0xe0, 0x30, 0x00, 0x03, 0x7f, 0xfb, 0x60, 0xdb, 0xff, 0xd8, 0x00, 0x01, 0x9f, 0x06, 0x60, 
	0x30, 0x00, 0x03, 0x7f, 0xfb, 0x60, 0xdb, 0xff, 0xf8, 0x00, 0x01, 0xc0, 0x0c, 0x40, 0x70, 0x00, 
	0x03, 0xff, 0xfb, 0x60, 0xdb, 0xff, 0xf8, 0x00, 0x00, 0xe0, 0x08, 0xc0, 0x60, 0x00, 0x03, 0xff, 
	0xfb, 0x60, 0xdb, 0xff, 0xe0, 0x00, 0x00, 0xe0, 0x18, 0x80, 0xe0, 0x00, 0x00, 0xff, 0xfb, 0x60, 
	0xdb, 0xff, 0xe0, 0x00, 0x00, 0x70, 0x11, 0x81, 0xc0, 0x00, 0x00, 0xff, 0xfb, 0x60, 0xdb, 0xff, 
	0xe0, 0x00, 0x00, 0x38, 0x3f, 0x03, 0x80, 0x00, 0x00, 0xff, 0xfb, 0x60, 0xdb, 0xff, 0xe0, 0x00, 
	0x00, 0x1e, 0x00, 0x1f, 0x00, 0x00, 0x00, 0xff, 0xfb, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 
	0x80, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xf8, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00
  };

//  __      ______ _____ _____
//  \ \    / / __ \_   _|  __ \ "/"
//   \ \  / / |  | || | | |  | |
//    \ \/ /| |  | || | | |  | |
//     \  / | |__| || |_| |__| |
//      \/   \____/_____|_____/

//============

//      _ _                               
//   __| (_)_ __ ___  _ __ ___   ___ _ __ 
//  / _` | | '_ ` _ \| '_ ` _ \ / _ \ '__|
// | (_| | | | | | | | | | | | |  __/ |   
//  \__,_|_|_| |_| |_|_| |_| |_|\___|_|   

void dimmer() {  
  if (headlights == 1) {
    color = orange;
    color2 = red;
    }
  else {
    color = white;
    color2 = white;
   }
   //bitmaps                                      
   tft.drawBitmap(0, 0, oil_lamp, 47, 27, color);
   tft.drawBitmap(60, 67, battSmall, 20, 20, color);
   tft.drawBitmap(0, 67, coolant, 20, 20, color);
   tft.drawBitmap(0, 30, gauge, 128, 15, color);
   tft.drawBitmap(10,95,logo,107,31,color);
   //degrees C
   tft.drawChar(102,7,0x09,color2,black,2);
   tft.setTextSize(2);
   tft.setTextColor(color2);   
   tft.setCursor(112,13);
   tft.print("C");
   //oil temp gauge
   tft.setTextColor(color);      
   tft.setTextSize(1);
   tft.setCursor(21,39);
   tft.setCursor(0,57);
   tft.print(0);
   tft.setCursor(58,57);
   tft.print(85);
   tft.setCursor(110,57);
   tft.print(170);
}

//  _           _              _ _       
// | |__   __ _| |_/\   /\___ | | |_ ___ 
// | '_ \ / _` | __\ \ / / _ \| | __/ __|
// | |_) | (_| | |_ \ V / (_) | | |_\__ \ "/"
// |_.__/ \__,_|\__| \_/ \___/|_|\__|___/
                                      
void batVolts (void)
{
  batSensor = analogRead(batAnalogIn);
  batVoltage = (batSensor * ((r1+r2)/r2) ) * (aref/1024);
  //ADC reads 0-5v in 0-1023
  //Therefore, equation is 5/1023 for measured voltage
  //Multiply that (0.0048828125) by 3.672619048 for battery voltage
  batAvg = (batVoltage + batLast) / 2;
  batLast = batAvg;
}

//============

//void mode_counter_increase (void)
//{
//modeButton = 1;
//}

//============

//void mode_counter_decrease (void)
//{
//modeButton = 0;
//}

//============

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

//============

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars, ","); //NULL is after first delimiter, before first delimiter use tempChars
    int1 = atoi(strtokIndx);     // atoi = conver string to integer
    //following code to make it indent right - blanks numbers on screen when necessary
    //to fix in future
//        if (int1 >= 100){   
//      big1 = 1;
//      start = 1; //stops int1 being empty when program runs
//     }
//    else {
//      if (big1 == 1); {
//        change1 = 1;
//      }
//      big1 = 0;
//    }

    //2nd digit in serial sequence
    strtokIndx = strtok(NULL, ",");
    int2 = atoi(strtokIndx);

    //3rd digit etc..
    strtokIndx = strtok(NULL, ",");
    int3 = atoi(strtokIndx);
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

  analogReference(INTERNAL1V1); // use ianalogReference(INTERNAL)nternal voltage reference
                                //***CAUTION*** do not connect >1v to any analogRead pin!!!
  
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
  
  tft.fillScreen(black);
  
  //bitmaps
   tft.drawBitmap(0, 0, oil_lamp, 47, 27, white);
   tft.drawBitmap(60, 67, battSmall, 20, 20, white);
   tft.drawBitmap(0, 67, coolant, 20, 20, white);
   tft.drawBitmap(0, 30, gauge, 128, 15, white);
   tft.drawBitmap(10,95,logo,107,31,white);
   //degrees C
   tft.drawChar(102,7,0x09,white,black,2);
   tft.setTextSize(2);
   tft.setTextColor(white);   
   tft.setCursor(112,13);
   tft.print("C");
   //oil temp gauge
   tft.setTextColor(white);      
   tft.setTextSize(1);
   tft.setCursor(0,57);
   tft.print(0);
   tft.setCursor(58,57);
   tft.print(85);
   tft.setCursor(110,57);
   tft.print(170);

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
  headlights = digitalRead(headlightSignal); //check if can delete - if checked in 'if' statement below
  //Serial.println(headlights);
  if (headlights == 1) {                     
    analogWrite(Lite,50);
    headlightStatus = 1;
  }
  else {
    analogWrite(Lite,255);
    headlightStatus = 0;
  }
  if (headlights != headlightStatus) {
    dimmer();
  }

  millis10 = millis();
 }
 
 if ( millis() >= millis200 + 200 ) {
 
  recvWithStartEndMarkers();
  
  //voltage
  batVolts(); 
  // Shift the decimal point right two digits and round off to an integer.
  int voltage = (batAvg * 100.0) + 0.5;
  // Extract each digit with the 'modulo' operator (%)
  char nonesdigit = '0' + ((voltage / 1000)% 10);
  char onesDigit = '0' + ((voltage / 100) % 10);
  char tensDigit =  '0' + ((voltage / 10) % 10);
  char hundredsDigit =  '0' + (voltage % 10);

  
  //collect data from softSerial
  if (newData == true) {
      //nudat = 1;  //debugging
      //Serial.println(nudat);
      strcpy(tempChars, receivedChars);
          // this temporary copy is necessary to protect the original data
          //   because strtok() used in parseData() replaces the commas with \0
      parseData();
      newData = false;
      }
      
//   //blank bigger numbers for oil temp
// //to fix in future  
//    if (change1 == 1 && start == 1) {
//    tft.fillRect(50,0,12,28,black); //(x,y,w,h,color)
//    change1 = 0;
//    } 
 
 //Print oil temp gauge - horizontal bar
   if (int1 < 85) {
   flash = 0;
   tft.fillRect(0,47,2,8,blueDark);
   tft.fillRect(2,47,(int1*0.7529),8,blueDark);
   tft.fillRect(int1*0.7529+1,47,(128-(int1*0.7529)),8,black);
   }
   if (int1 >= 85 && int1 <=129) {
   flash = 0;
   tft.fillRect(0,47,2,8,greenDark);
   tft.fillRect(2,47,(int1*0.7529),8,greenDark);
   tft.fillRect(int1*0.7529+1,47,(128-(int1*0.7529)),8,black);
   }
   if (int1 >= 130) {
   flashActivate = 1;
   tft.fillRect(0,47,2,8,red);
   tft.fillRect(2,47,(int1*0.7529),8,red);
   tft.fillRect(int1*0.7529+1,47,(128-(int1*0.7529)),8,black);
   } 
  
  //print data
   //flashing oil temp
   if (millis() >= millis50 + 50){
    if (flash == 0) {
     tft.setTextColor(color2,black);
     }
    if (flash == 1) {
     tft.setTextColor(black,black);
     tft.fillRect(68,7,35,22,black);
    }
    if (flashActivate == 1) {
     flash = !flash;
    }
    millis50 = millis();
   }

  //oil temp - int1
   tft.setTextSize(3);
   if (int1 < 10) {
    if (size != 1) {
     tft.fillRect(68,0,12,28,black); //(x,y,w,h,color)
     tft.fillRect(50,0,12,28,black); //(x,y,w,h,color)
    }
    size = 1;
    }
   if (int1 >= 10 && int1 < 100) {
    if (size != 2) {
     tft.fillRect(50,0,12,28,black); //(x,y,w,h,color)
    }
    size = 2;
    }
   if (int1 >= 100) {
    size = 3;
   }

   if (size == 1 ) { 
    tft.setCursor(86,5);
    tft.print(int1);
    }
   if (size == 2) {
    tft.setCursor(68,5); 
    tft.print(int1);
    }
   if (size == 3 ) { 
    tft.setCursor(50,5);
    tft.print(int1);
    }

  //coolant temp - int2
   tft.setTextSize(2);
   tft.setTextColor(color2,black);  
   if (int2 != 0) {
    if (int2 <100 ) {
     tft.fillRect(46,72,12,14,black);
    }
   tft.setCursor(23,72);
   tft.print(int2);
  }
  
 //battery voltage - int3
  tft.setCursor(82,72);
  tft.print(nonesdigit);
  tft.print(onesDigit);
  tft.setTextSize(1);
  tft.setCursor(104,79);
  tft.print(".");
  tft.print(tensDigit);
  tft.print(hundredsDigit);
    
  
  millis200 = millis();
   } 
}
