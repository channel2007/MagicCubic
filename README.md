# MagicCubic(魔方)
![alt MagicCubic](https://github.com/channel2007/MagicCubic/blob/master/Images/MagicCubic.png "MagicCubic")

# 前言
* 自製螢幕投射裝置(使用分光棱鏡)

# 準備材料
* D1MINI KIT ESP32 開發板
* TFT1.3寸IPS彩色顯示模組240X240 ST7789 3.3V
* 分光棱鏡25.4x25.4x25.4mm
* Micro SD卡模組
* 五向導航按鍵模組
* 400孔麵包板
* 杜邦線

# 線路圖
![alt WiringDiagram](https://github.com/channel2007/MagicCubic/blob/master/Images/WiringDiagram.png "WiringDiagram")

# 安裝函數庫
* [Arduino_GFX](https://github.com/moononournation/Arduino_GFX)
  * 安裝完畢開啟(Arduino_ST7789.cpp)檔案
  * 找到 r = ST7789_MADCTL_RGB; 修改為 r = ST7789_MADCTL_MX | ST7789_MADCTL_RGB;
 
* [PNGdec](https://github.com/bitbank2/PNGdec)

# 目錄功能說明
* MagicCubic_GIF
  * GIF撥放器
* MagicCubic_Clock
  * 電子時鐘    

# 讓MagicCubic(魔方)連上WIFI
* 請建立一檔案config.txt，在裡面輸入WIFI名稱與WIFI密碼存檔後將此檔案放入Micro SD卡內，並插入機器後方的Micro SD插槽
![alt Wifi](https://github.com/channel2007/MagicCubic/blob/master/Images/Wifi.png "Wifi")

# 官方紛絲團 
[無限升級](https://www.facebook.com/unlimited.upgrade/posts/2840132506240869?notif_id=1617421138749926&notif_t=page_post_reaction&ref=notif)

# 官方Youtube
[無限升級](https://www.youtube.com/channel/UC4reRKznNk1CcjZfxKUdMuw)
