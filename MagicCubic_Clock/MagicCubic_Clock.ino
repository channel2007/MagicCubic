#include <WiFi.h>
#include <Arduino_GFX_Library.h>
#include <SPIFFS.h>
#include <SD.h>

#include <PNGdec.h>

const char* ssid     = "";
const char* password = "";
const char* ntpServer = "pool.ntp.org";

char tn[10][10]  = {{"/tn_0.png"}, {"/tn_1.png"}, {"/tn_2.png"}, {"/tn_3.png"}, {"/tn_4.png"}, {"/tn_5.png"}, {"/tn_6.png"}, {"/tn_7.png"}, {"/tn_8.png"}, {"/tn_9.png"}};
char tns[10][11] = {{"/tns_0.png"}, {"/tns_1.png"}, {"/tns_2.png"}, {"/tns_3.png"}, {"/tns_4.png"}, {"/tns_5.png"}, {"/tns_6.png"}, {"/tns_7.png"}, {"/tns_8.png"}, {"/tns_9.png"}};
char wn[7][10] = {{"/wn_0.png"}, {"/wn_1.png"}, {"/wn_2.png"}, {"/wn_3.png"}, {"/wn_4.png"}, {"/wn_5.png"}, {"/wn_6.png"}};

const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 3600;

static Arduino_DataBus *bus = new Arduino_ESP32SPI(
  27 /* DC */, 
  -1 /* CS */, 
  14 /* 18 SCK */, 
  13 /* 23 SDA(MOSI) */, 
  -1 /* MISO */, 
 VSPI /* spi_num */);

static Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 
  26 /* RST */, 
  0 /* rotation */,
  true /* IPS */,
  240 /* width */, 
  240 /* height */,
  0 /* col offset 1 */, 
  0 /* row offset 1 */);

PNG png;
int16_t w, h, xOffset, yOffset;
File pngFile;

void *myOpen(const char *filename, int32_t *size)
{
/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
  pngFile = SD.open(filename, "r");
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
  //pngFile = LittleFS.open(filename, "r");
  pngFile = SD.open(filename, "r");
#elif defined(ESP32)
  // pngFile = FFat.open(filename, "r");
  //pngFile = LittleFS.open(filename, "r");
  pngFile = SPIFFS.open(filename, "r");
  //pngFile = SD.open(filename, "r");
#elif defined(ESP8266)
  //pngFile = LittleFS.open(filename, "r");
  pngFile = SD.open(filename, "r");
#else
  pngFile = SD.open(filename, FILE_READ);
#endif

  if (!pngFile || pngFile.isDirectory())
  {
    Serial.println(F("ERROR: Failed to open file for reading"));
  }else{
    *size = pngFile.size();
    Serial.printf("Opened '%s', size: %d\n", filename, *size);
  }

  return &pngFile;
}
void myClose(void *handle){
  if (pngFile)
    pngFile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
  if (!pngFile)
    return 0;
  return pngFile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position){
  if (!pngFile)
    return 0;
  return pngFile.seek(position);
}
// Function to draw pixels to the display
void PNGDraw(PNGDRAW *pDraw){
  uint16_t usPixels[240];
  uint8_t usMask[240];

  // Serial.printf("Draw pos = 0,%d. size = %d x 1\n", pDraw->y, pDraw->iWidth);
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  png.getAlphaMask(pDraw, usMask, 1);
  gfx->draw16bitRGBBitmap(xOffset, yOffset + pDraw->y, usPixels, usMask, pDraw->iWidth, 1);
}
void PNGDraw(char *pngName, int32_t x, int32_t y){
  if (!SPIFFS.begin()){
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }else{
    unsigned long start = millis();
    int rc;
    rc = png.open( pngName, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS)
    {
      int16_t pw = png.getWidth();
      int16_t ph = png.getHeight();

      xOffset = x;
      yOffset = y;

      rc = png.decode(NULL, 0);
      Serial.printf("Draw offset: (%d, %d), time used: %lu\n", xOffset, yOffset, millis() - start);
      Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
      png.close();
    }else{
      Serial.println("png.open() failed!");
    }
  }
}

void split( char *src, const char *separator, char **dest, int *num){
  char *pNext;
  int count=0;
  if(src==NULL || strlen(src)==0)
    return;
  if(separator==NULL || strlen(separator)==0)
    return;
  pNext = (char *)strtok(src,separator);
  while(pNext!=NULL){
    *dest++=pNext;
    ++count;
    pNext=(char *)strtok(NULL,separator);
  }
  *num=count;
}

void showMessage(int16_t x, int16_t y,  int16_t rx, int16_t ry, int16_t rw, int16_t rh, uint8_t sx, uint8_t sy, uint8_t pixMargin, String str, bool clearBG=true){
  // Init Display  
  if(clearBG){
    gfx->begin();
    gfx->fillScreen(BLACK);
  }    
  gfx->fillRoundRect(rx, ry, rw, rh, 0, RED);
  gfx->setTextColor(WHITE);
  gfx->setTextSize( sx /* x scale */, sy /* y scale */, pixMargin/* pixel_margin */);
  gfx->setCursor(x, y);  
  gfx->println(str);
}

char secondTemp[3];
void printLocalTime(bool ud=false){  
  struct tm timeinfo;
  
  char hour[3];
  char minute[3];
  char second[3];
  char year[6];
  char month[3];
  char day[3];
  char week[3];
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  strftime(secondTemp, 3, "%S", &timeinfo);
  if(second==secondTemp)
    return;

  strftime(  year, 6, "%Y", &timeinfo);
  strftime( month, 3, "%m", &timeinfo);
  strftime(   day, 3, "%d", &timeinfo);
  strftime( week, 3, "%w", &timeinfo);
  strftime(  hour, 3, "%H", &timeinfo);
  strftime(minute, 3, "%M", &timeinfo);
  strftime(second, 3, "%S", &timeinfo);
  
  if (!SPIFFS.begin()){
    Serial.println(F("ERROR: File System Mount Failed!"));
  }else{ 
    PNGDraw(tns[(int)(second[0]-'0')], 45, 98);    
    PNGDraw(tns[(int)(second[1]-'0')], 84, 98);
    if((second[0]=='0' && second[1]=='0') || ud){
      PNGDraw( tn[(int)(minute[0]-'0')],  2, 147);
      PNGDraw( tn[(int)(minute[1]-'0')], 80, 147);    
      if((minute[0]=='0' && minute[1]=='0') || ud){
        PNGDraw( tn[(int)(hour[0]-'0')],  2, 0);
        PNGDraw( tn[(int)(hour[1]-'0')], 80, 0);
        PNGDraw(tns[(int)(month[0]-'0')], 163, 1);
        PNGDraw(tns[(int)(month[1]-'0')], 202, 1);
        PNGDraw(tns[(int)(day[0]-'0')], 163,47);
        PNGDraw(tns[(int)(day[1]-'0')], 202,47);
        PNGDraw("/wf.png", 176,93);
        PNGDraw(wn[(int)(week[0]-'0')], 176, 195);
      }
    }
  }
}

void setup(){
  int   filesMax=0;
  char *filesList[32]={0};
  char charBuf[256];  
  bool inSDCard = true;

  Serial.begin(115200);

  if(!SD.begin(5)){
    showMessage( 0, 104, 0, 75, 240, 85, 4, 4, 1, "No SD Card");
    inSDCard = SD.begin();    
    while (!inSDCard){    
      delay(500);
    }    
  }else{
    if(!SD.exists("/config.txt")){
      showMessage( 2, 104, 0, 75, 240, 85, 3, 3, 1, "No config.txt");
      while (!SD.exists("/config.txt")) {    
        delay(500);
      }
    }else{      
      File configFile = SD.open("/config.txt", FILE_READ);
      configFile.readString().toCharArray(charBuf, 256);
      split( charBuf,"\r\n", filesList, &filesMax);
      if(filesList[0]!="")
        ssid = filesList[0];
      if(filesList[1]!="")
        password = filesList[1];
      configFile.close();
            
      // Connect to Wi-Fi
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      showMessage( 12, 111, 0, 75, 240, 85, 2, 2, 2, "Connect WiFi...");
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("WiFi connected.");
      showMessage( 12, 111, 0, 75, 240, 85, 2, 2, 2, "Connect WiFi... ok", false);
      delay(500);

      gfx->begin();
      gfx->fillScreen(BLACK);
            
      // Init and get the time
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      printLocalTime(true);

      // disconnect WiFi as it's no longer needed.
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);           
    }     
  }

}

void loop(){
  delay(200);
  printLocalTime();
}
