/*
DATOS DE INICIO EN TODO.txt
this IS THE NEW BRANCH!!
*/
#include <SPI.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h> // JPEG decoder library
#include "Wire.h"               //RTC DS3231 
#include <DHT.h>
#include "pixie0.h" //Nixie tube red
#include "pixie1.h" //Split Flap
#include "pixie2.h" //balloon
 
#define DS3231_I2C_ADDRESS 0x68 //RTC DS3231 

//DHT sensor es bidireccional no conectar en solo inputs pines
#define DHTPIN 12     //DHT pin
#define DHTTYPE    DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE);

///SPI HSPI para CD card.
#include <SD.h>
#define SD_MOSI      13
#define SD_MISO      5
#define SD_SCK       14
#define SD_CS_PIN   15//16  I THINK is not required, 'cause only device on separated HSPI bus.
SPIClass SPISD(HSPI); //instancia HSPI

//WS2812B 
#include <FastLED.h>
#define DATA_PIN    27
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    8
#define BRIGHTNESS          96
CRGB leds[NUM_LEDS];

TFT_eSPI tft = TFT_eSPI();

// Return the minimum of two values a and b
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

// Count how many times the image is drawn for test purposes
uint32_t icount = 0;

// Chip Select pins, CS pins are active low
#define unoScreenCS 16
#define dosScreenCS 17
#define tresScreenCS 19
#define cuatroScreenCS 25//21
#define cincoScreenCS 33//32//22  los cambie para correcta posición.
#define seisScreenCS 32//33

//Chip Select PINOUT EMPIEZO DEL ARRAY UNO PARA MAYOR COMODIDAD.
const int CS_PINS[7] = { -1, unoScreenCS, dosScreenCS, tresScreenCS, cuatroScreenCS, cincoScreenCS, seisScreenCS}; 

//ARRAY oF PROGMEM's
  //https://forum.arduino.cc/t/how-to-make-an-array-of-pointers-to-arrays-in-progmem/446103/4
  const uint8_t* const nixieTube[] PROGMEM = {nixie0, nixie1, nixie2, nixie3, nixie4, nixie5, nixie6, nixie7, nixie8,  nixie9,  nixiePoint,  nixieSemiColon};
  const uint8_t* const nixieFlap[] PROGMEM = {cero0, cero1, cero2, cero3, cero4, uno0, uno1, uno2, uno3,   
            dos0, dos1, dos2, dos3 ,   tres0, tres1, tres2, tres3,  cuatro0, cuatro1, cuatro2, cuatro3,  
            cinco0, cinco1, cinco2, cinco3, seis0, seis1, seis2, seis3,  siete0, siete1, siete2, siete3,
            ocho0, ocho1, ocho2, ocho3, nueve0, nueve1, nueve2 };
  const uint8_t* const nixieBalloon[] PROGMEM = {balloon0, peluche1, balloon2_1, balloon3_1, balloon4_1,  balloon5_1, balloon6, balloon7, balloon8,  balloon9 };
  const char* const nixieUser[] = { "/nixie0.jpg", "/nixie1.jpg", "/nixie2.jpg", "/nixie3.jpg", "/nixie4.jpg", "/nixie5.jpg", "/nixie6.jpg", "/nixie7.jpg", "/nixie8.jpg", "/nixie9.jpg"  };

//GLOBAL VARIABLES.
  int retardo = 80; //ms delay, SOLO PARA SPLIT FLAP DIAL
  int  h0,h1,m0,m1,s0,s1=0;  //Estados de cada pantalla

//DECLARATION PROTOTYPE
void setDisplay(int NIXIE, int NUMBER);
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year);
void drawSdJpeg(const char *filename, int xpos, int ypos);
void renderJPEG(int xpos, int ypos);

//PROTOYPE DECLARATION OF FUNCTIONs
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);
void updateDisplay(int nixie, int numero);



void setup() {
  Serial.begin(115200);
  Serial.print("PIXIE ");

 // The CS pins will be outputs
  pinMode(unoScreenCS,OUTPUT);  
  pinMode(dosScreenCS,OUTPUT); 
  pinMode(tresScreenCS,OUTPUT); 
  pinMode(cuatroScreenCS,OUTPUT); 
  pinMode(cincoScreenCS,OUTPUT); 
  pinMode(seisScreenCS,OUTPUT);  

/// SETUP SDCARD 
      SPISD.begin(SD_SCK, SD_MISO, SD_MOSI);
      if (!SD.begin(SD_CS_PIN,SPISD)) {  //SD_CS_PIN this pin is just the dummy pin since the SD need the input 
      Serial.println(F("SD CARD failed!"));
      //return;
      } else Serial.println(F("SD read!"));
  Serial.println("initialisation done.");  
 
  // enable screens for  general initialisation
  digitalWrite(unoScreenCS,0); 
  digitalWrite(dosScreenCS,0);
  digitalWrite(tresScreenCS,0);
  digitalWrite(cuatroScreenCS,0);
  digitalWrite(cincoScreenCS,0);
  digitalWrite(seisScreenCS,0);

  tft.begin();
  tft.init();
  tft.setRotation(0);
  //tft.fillScreen(TFT_BLACK);
  tft.fillScreen(TFT_WHITE);

//Todas pantallas están activas solo poner una imagen cero.
 setDisplay( 1, 0 ); 
 
  //Disable all Screens
  digitalWrite(unoScreenCS,1);
  digitalWrite(dosScreenCS,1);
  digitalWrite(tresScreenCS,1);
  digitalWrite(cuatroScreenCS,1);
  digitalWrite(cincoScreenCS,1);
  digitalWrite(seisScreenCS,1);

  Wire.begin();

//SET RTC TIME 
//seconds, minutes, hours, dayOfWeek (1=Sunday, 7=Saturday),dayOfMonth, month, year (Byte)
//setDS3231time(00,24,9,3,18,3,25);    
  
//DHT11
  dht.begin();

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);  // set master brightness control
  //apaga LEDs cuando inicie...
  FastLED.clear();  
  FastLED.show(); 
}


void loop() {
  //GET TIME RTC_DS3231
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
 // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,  &year);
  //Serial.printf("second %d, min %d, hour %d, day %d, month %d, year %d, \n\r" , second, minute, hour, dayOfMonth, month, year);

//despliega TEMP cada segundo
if (  int(second%10) != s0) {   
  //read temperature and humidity
  int t = dht.readTemperature();
  int h = dht.readHumidity();
  if (isnan(h) || isnan(t)) { Serial.println("Failed to read from DHT sensor!"); }
  Serial.printf("Temp: %d, Hmd: %d\n", t,h);
 }   //seg

 
//NOTA DESCOMENTAR UNO Y SELECCIONAR EL ARREGLO EN setDisplay()
//NIXIE_TUBE, BALLOON,//USER NIXIE FROM SD CARD

/* */
//HORA
if (  int(hour/10) != h1) { h1= int(hour/10); setDisplay( 1, int(hour/10) );   Serial.println("h1"); }   //NIXIE, NUMBER
if (  int(hour%10) != h0) { h0= int(hour%10); setDisplay( 2, int(hour%10) );  Serial.println("h0");  }   //NIXIE, NUMBER
//MIN
if (  int(minute/10) != m1) { m1= int(minute/10); setDisplay( 3, int(minute/10) );   Serial.println("m1"); }   //NIXIE, NUMBER
if (  int(minute%10) != m0) { m0= int(minute%10); setDisplay( 4, int(minute%10) );  Serial.println("m0");  }   //NIXIE, NUMBER
//SEC
if (  int(second/10) != s1) { s1= int(second/10); setDisplay( 5, int(second/10) );   Serial.println("s1"); }   //NIXIE, NUMBER
if (  int(second%10) != s0) { s0= int(second%10); setDisplay( 6, int(second%10) );  Serial.println("s0");  
    Serial.printf("dayofweek %d, second %d, min %d, hour %d, day %d, month %d, year %d, \n\r" , dayOfWeek, second, minute, hour, dayOfMonth, month, year);
}   //NIXIE, NUMBER


/*  
 //NIXIE1 SPLIT_FLAP
//HORA
if (  int(hour/10) != h1) { h1= int(hour/10); updateDisplay( 1, int(hour/10) );   Serial.println("h1"); }   //NIXIE, NUMBER
if (  int(hour%10) != h0) { h0= int(hour%10); updateDisplay( 2, int(hour%10) );  Serial.println("h0");  }   //NIXIE, NUMBER
//MIN
if (  int(minute/10) != m1) { m1= int(minute/10); updateDisplay( 3, int(minute/10) );   Serial.println("m1"); }   //NIXIE, NUMBER
if (  int(minute%10) != m0) { m0= int(minute%10); updateDisplay( 4, int(minute%10) );  Serial.println("m0");  }   //NIXIE, NUMBER
//SEC
if (  int(second/10) != s1) { s1= int(second/10); updateDisplay( 5, int(second/10) );   Serial.println("s1"); }   //NIXIE, NUMBER
if (  int(second%10) != s0) { s0= int(second%10); updateDisplay( 6, int(second%10) );  Serial.println("s0");  
    Serial.printf("dayofweek %d, second %d, min %d, hour %d, day %d, month %d, year %d, \n\r" , dayOfWeek, second, minute, hour, dayOfMonth, month, year);
}   //NIXIE, NUMBER

*/

//LEDs SECOND BLINKING
  if(second%2 == 0) {  //PAR
    //leds[7] = CRGB::Black; 
    //leds[6] = CRGB::Black; 
    leds[5] = CRGB::Black; 
    leds[2] = CRGB::Black; 
    FastLED.show(); 
  } else { 
    //leds[7] = CRGB::Blue;
    //leds[6] = CRGB::Green;
    leds[5] = CRGB::Red;
    leds[2] = CRGB::Red;
    FastLED.show();  
  }
}//Loop



/////////////////////////////FUNCTIONS/////////////////////////////////////


// Draw a JPEG on the TFT pulled from a program memory array
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

  JpegDec.decodeArray(arrayname, array_size);
  //jpegInfo(); // Print information from the JPEG file (could comment this line out)
  renderJPEG(x, y);
  //Serial.println("#########################");
}

//Despliega en Pantallas
 void setDisplay(int NIXIE, int NUMBER){
    digitalWrite( CS_PINS[NIXIE] ,0); 
      drawArrayJpeg( nixieTube[NUMBER] , sizeof( nixie0 ) , 0, 0); 
      //drawArrayJpeg( nixieBalloon[NUMBER] , sizeof( nixie0 ) , 0, 0); 
      //drawSdJpeg( nixieUser[NUMBER] , 0, 0); //SDCARD
      //drawArrayJpeg( nixieFlap[NUMBER] , sizeof( nixie0 ) , 0, 0); //SELECCIONAR Arriba updateDisplay()
    digitalWrite( CS_PINS[NIXIE] ,1); 
 }





// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void renderJPEG(int xpos, int ypos) {

  // retrieve information about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.readSwappedBytes()) {
	  
    // save a pointer to the image block
    pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {
      tft.pushRect(mcu_x, mcu_y, win_w, win_h, pImg);
    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  //Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  //Serial.println(F(""));
}

// Print image information to the serial port (optional)
void jpegInfo() {
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F(  "Width      :")); Serial.println(JpegDec.width);
  Serial.print(F(  "Height     :")); Serial.println(JpegDec.height);
  Serial.print(F(  "Components :")); Serial.println(JpegDec.comps);
  Serial.print(F(  "MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F(  "MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F(  "Scan type  :")); Serial.println(JpegDec.scanType);
  Serial.print(F(  "MCU width  :")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F(  "MCU height :")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}

// Show the execution time (optional)
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}



////////////MOMENTANEO PARA SPLIT FLAP NUMBERS/////////////////

void updateDisplay(int nixie, int numero) {
  if ( numero == 0){ //secuencia 0
    setDisplay(nixie, 37 );  delay(retardo);
    setDisplay(nixie, 38 );  delay(retardo);
    setDisplay(nixie, 39 );  delay(retardo);
    setDisplay(nixie, 0 );  delay(retardo);
  }

  if ( numero == 1){ //secuencia 1
    setDisplay(nixie, 1 );  delay(retardo);
    setDisplay(nixie, 2 );  delay(retardo);
    setDisplay(nixie, 3 );  delay(retardo);
    setDisplay(nixie, 4 );  delay(retardo);
  }

  if ( numero == 2){ //secuencia 2
    setDisplay(nixie, 5 );  delay(retardo);
    setDisplay(nixie, 6 );  delay(retardo);
    setDisplay(nixie, 7 );  delay(retardo);
    setDisplay(nixie, 8 );  delay(retardo);
  }

  if ( numero == 3){ //secuencia 3
    setDisplay(nixie, 9 );  delay(retardo);
    setDisplay(nixie, 10 );  delay(retardo);
    setDisplay(nixie, 11 );  delay(retardo);
    setDisplay(nixie, 12 );  delay(retardo);
  }

  if ( numero == 4){ //secuencia 4
    setDisplay(nixie, 13 );  delay(retardo);
    setDisplay(nixie, 14 );  delay(retardo);
    setDisplay(nixie, 15 );  delay(retardo);
    setDisplay(nixie, 16 );  delay(retardo);
  }

  if ( numero == 5){ //secuencia 5
    setDisplay(nixie, 17 );  delay(retardo);
    setDisplay(nixie, 18 );  delay(retardo);
    setDisplay(nixie, 19 );  delay(retardo);
    setDisplay(nixie, 20 );  delay(retardo);    
  }

  if ( numero == 6){ //secuencia 6
    setDisplay(nixie, 21 );  delay(retardo);
    setDisplay(nixie, 22 );  delay(retardo);
    setDisplay(nixie, 23 );  delay(retardo);
    setDisplay(nixie, 24 );  delay(retardo);    
  }

  if ( numero == 7){ //secuencia 7
    setDisplay(nixie, 25 );  delay(retardo);
    setDisplay(nixie, 26 );  delay(retardo);
    setDisplay(nixie, 27 );  delay(retardo);
    setDisplay(nixie, 28 );  delay(retardo);   
  }

  if ( numero == 8){ //secuencia 8
    setDisplay(nixie, 29 );  delay(retardo);
    setDisplay(nixie, 30 );  delay(retardo);
    setDisplay(nixie, 31 );  delay(retardo);
    setDisplay(nixie, 32 );  delay(retardo);   
  }

  if ( numero == 9){ //secuencia 9
    setDisplay(nixie, 33 );  delay(retardo);
    setDisplay(nixie, 34 );  delay(retardo);
    setDisplay(nixie, 35 );  delay(retardo);
    setDisplay(nixie, 36 );  delay(retardo);   
  }
}


////////  RTC CONVERSIONS /////////////////
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}

//SET TIME TO RTC
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year){
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

// READ FROM RTC
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}


////////////////  SDCARD //////////////
//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    //jpegInfo();
    // render the image onto the screen at given coordinates
    //jpegRender(xpos, ypos);
    renderJPEG(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

