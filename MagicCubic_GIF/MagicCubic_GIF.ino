#include <WiFi.h>
#include <Arduino_GFX_Library.h>
#include <SPIFFS.h>
#include <SD.h>

#include "GifClass.h"
static GifClass gifClass;

const char* ssid = "";
const char* password = "";

static TaskHandle_t LoopGifTask;   

static File     file;
static bool     opened=false;
static byte     gifState=0;
static byte     gifStateTemp=0;

static String   gifFileListString="";
static int      gifFilesNow=0;

static String   sGifName="";
static String   sGifNameTemp="";

static int  buttonState[5]={0,0,0,0,0};
static bool buttonDownState[5]={false,false,false,false,false};

static int32_t sButtonPress;

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

String listDir(fs::FS &fs, const char * dirname, uint8_t levels, bool outSize=true){
    String response = "";

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return "";
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return "";
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
            if(outSize)
              response += (String(file.name())+","+String(file.size())+"|");
            else
              response += String(file.name())+"|";
        }
        file = root.openNextFile();
    }
    if(response=="")
      return "0";
    else
      return response;
}

String gifFileName(byte s=0){
  int   filesMax=0;
  char *filesList[32]={0};
  char charBuf[256];  
  gifFileListString.toCharArray(charBuf, 256);
  split( charBuf,"|", filesList, &filesMax);

  if(s==1){
    gifFilesNow--;
    if(gifFilesNow<0)
      gifFilesNow = filesMax-1;      
  }else if(s==2){
    gifFilesNow++;
    if(gifFilesNow>(filesMax-1)){
      gifFilesNow = 0;
    }
  }
  Serial.println(gifFilesNow);
  return filesList[gifFilesNow];
}

void showMessage(int16_t x, int16_t y,  int16_t rx, int16_t ry, int16_t rw, int16_t rh, uint8_t sx, uint8_t sy, uint8_t pixMargin, String str, bool clearBG=true){
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

void updateGifFiles(){    
  gifFileListString = listDir(SPIFFS, "/", 0, false);
}

void setGifState(byte s, byte update=0){  
  if(gifState==s && update==0)
    return;

  gifState=s;  
  if(update==2)
    return;

  if(s==1){
    gfx->fillScreen(BLACK);
  }else{
    if(gifState==0){    
      showMessage( 18, 104, 0, 75, 240, 100, 7, 7, 4, "PAUSE", false);    
    }else if(gifState==2){
      showMessage( 18, 104, 0, 75, 240, 100, 6, 6, 4, "NO GIF", false);    
    }else if(gifState==3){
      showMessage( 18, 104, 0, 75, 240, 100, 6, 6, 4, "UPDATE", false);    
    }else if(gifState==4){    
      showMessage( 6, 87, 0, 75, 240, 85, 2, 2, 2, "Upload GIF...", false);
      gfx->setCursor(6, 111);
      gfx->println(WiFi.localIP());  
      gfx->setCursor(6, 135);
      gfx->println("gif.local");    
    }else if(gifState==5){    
      showMessage( 6, 87, 0, 75, 240, 85, 2, 2, 2, "Can't connect to", false);
      gfx->setCursor(6, 111);
      gfx->println("wifi...");  
    }
  }
}

void setup()
{
  int   filesMax=0;
  char *filesList[32]={0};
  char charBuf[256];  
  bool inSDCard = true;

	Serial.begin(115200);

  pinMode(17, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);

  if(!SD.begin()){
    showMessage( 0, 104, 0, 75, 240, 85, 4, 4, 1, "No SD Card");
    delay(2000);
    
  }else{
    if(!SD.exists("/config.txt")){
      showMessage( 2, 104, 0, 75, 240, 85, 3, 3, 1, "No config.txt");
      delay(2000);
    
    }else{
      File configFile = SD.open("/config.txt", FILE_READ);
      configFile.readString().toCharArray(charBuf, 256);
      split( charBuf,"\r\n", filesList, &filesMax);

      if(filesList[0]!="")
        ssid = filesList[0];
      if(filesList[1]!="")
        password = filesList[1];
      configFile.close();

      WiFi.begin(ssid, password);  
      showMessage( 12, 111, 0, 75, 240, 85, 2, 2, 2, "Connect WiFi...");
      while (WiFi.status() != WL_CONNECTED) {
          delay(250);
      }
      showMessage( 12, 111, 0, 75, 240, 85, 2, 2, 2, "Connect WiFi... ok", false);
      delay(500);
    }
  }

  if (!SPIFFS.begin())
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
    exit(0);
  }

  xTaskCreatePinnedToCore(
                    loopGif,
                    "LoopGifTask",
                    10000,
                    NULL,
                       1,
            &LoopGifTask,
                        1
  );                 

}

void loopGif(void * pvParameters){
  updateGifFiles();
  sGifName=sGifNameTemp=gifFileName();
  gfx->fillScreen(BLACK);
  if(gifFileListString == "0")
    setGifState(2);
  else
    setGifState(1);

  while (true){  
    delay(1);
    if(gifState==1){      
      if(gifFileListString != "0"){
        if(sGifName!=sGifNameTemp){
          gfx->fillScreen(BLACK);
        }
        sGifName=sGifNameTemp;
        File gifFile = SPIFFS.open( sGifName, "r");      
        if (!gifFile || gifFile.isDirectory())
        {
          Serial.println(F("ERROR: open gifFile Failed!"));
          gfx->println(F("ERROR!"));
        }else{
          gd_GIF *gif = gifClass.gd_open_gif(&gifFile);
          if (!gif)
          {
            Serial.println(F("gd_open_gif() failed!"));
          }else{
            uint8_t *buf = (uint8_t *)malloc(gif->width * gif->height);
            if (!buf)
            {
              Serial.println(F("buf malloc failed!"));
            }
            else
            {
              int16_t x = (gfx->width() - gif->width) / 2;
              int16_t y = (gfx->height() - gif->height) / 2;

              //Serial.println(F("GIF video start"));
              int32_t t_fstart, t_delay = 0, t_real_delay, delay_until;
              int32_t res = 1;
              int32_t duration = 0, remain = 0;
              while (res > 0)
              {
                t_fstart = millis();
                t_delay = gif->gce.delay * 10;
                res = gifClass.gd_get_frame(gif, buf);
                if (res < 0)
                {
                  Serial.println(F("ERROR: gd_get_frame() failed!"));
                  break;
                }
                else if (res > 0)
                {
                  gfx->drawIndexedBitmap(x, y, buf, gif->palette->colors, gif->width, gif->height);

                  t_real_delay = t_delay - (millis() - t_fstart);
                  duration += t_delay;
                  remain += t_real_delay;
                  delay_until = millis() + t_real_delay;
                  while (millis() < delay_until)
                  {
                    delay(1);
                    if((gifState!=1)||(sGifName!=sGifNameTemp))
                      break;
                  }
                }
                if((gifState!=1)||(sGifName!=sGifNameTemp)){
                  setGifState(gifState, 1);
                  break;
                }
              }
              gifClass.gd_close_gif(gif);
              free(buf);
            }
          }
        }    

      }else{
        gifState=2;
      }
    }
  }
}

void loop()
{
  delay(1);

  buttonState[0] = digitalRead(17);
  buttonState[1] = digitalRead(16);
  buttonState[2] = digitalRead(15);
  buttonState[3] = digitalRead(32);
  buttonState[4] = digitalRead(12);
    
  if(buttonState[0] == LOW){
    if(!buttonDownState[0]){
      if(gifState==1){ 
        sGifNameTemp=gifFileName(2);
      }      
      buttonDownState[0] = true;
    }
  }else{
    buttonDownState[0] = false;
  }

  if(buttonState[1] == LOW){
    if(!buttonDownState[1]){
      buttonDownState[1] = true;
    }
  }else{
    buttonDownState[1] = false;
  }

  if(buttonState[2] == LOW){
    if(!buttonDownState[2]){
      if(gifState==1){
        sGifNameTemp=gifFileName(1);
      }
      buttonDownState[2] = true;
    }
  }else{
    buttonDownState[2] = false;
  }

  if(buttonState[3] == LOW){
    if(!buttonDownState[3]){
      if(gifState==1){
        setGifState(0,2);
      }else if(gifState==0 || gifState==4 || gifState==5){
        if(gifFileListString=="0")
          setGifState(2);
        else
          setGifState(1);
      }
      buttonDownState[3] = true;
      sButtonPress = millis();      
    }
    int32_t t = millis()-sButtonPress;
    if( t >500 && (gifState==0||gifState==2)){
      if(WiFi.status() == WL_CONNECTED)
        setGifState(4);
      else
        setGifState(5);
    }
  }else{
    buttonDownState[3] = false;
  }

  if(buttonState[4] == LOW){
    if(!buttonDownState[4]){
      buttonDownState[4] = true;
    }
  }else{
    buttonDownState[4] = false;
  }

}
