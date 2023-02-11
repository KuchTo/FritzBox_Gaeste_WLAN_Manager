#include <Arduino.h>
#include <U8g2lib.h>
#include "qrcode.h"

#define ESPALEXA_ASYNC //it is important to define this before #include <Espalexa.h>!
#include <Espalexa.h>                 
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESP32Time.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <esp_task_wdt.h>
#include <EasyNTPClient.h>

const char WIFI_SSID[] = "MyWLANSSID";   // <-- ändern
const char WIFI_PASSWORD[] = "MyWLANPassword"; // <-- ändern
const char DEVICE_NAME[] = "GuestWLAN_Sw";

#define LCD_BACKLIGHT_PIN 4 
#define AlexaDeviceName "GUESTWLAN_Switch"
#define ESPALEXA_MAXDEVICES 2   // set maximum devices add-able to Espalexa

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

 
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22); // ESP32
QRCode qrcode;

// Global Variables for Basic Functions
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long WiFiWatchdogpreviousMillis = 0;
unsigned long RFWeatherpreviousMillis = 0;
unsigned long PushButtonpreviousMillis = 0;
bool debug = false;
bool WLANState = false;
bool WLANStateHelp = true;
#include "BasicFunctions.h"
#include "Webpages.h"
#include "TR64.h"

void setup() 
{
  u8g2.begin(); 
  delay(200);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(0,10, "Booting...");
  u8g2.sendBuffer();          // transfer internal memory to the display
  u8g2.setFont(u8g2_font_4x6_tf);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);    // sets the LCD_BACKLIGHT_PIN as output
  WLANState  = false;
  WLANStateHelp = true;
  u8g2.drawStr(0,25, "3. -Start WiFi");
  u8g2.sendBuffer();          // transfer internal memory to the display
  Serial.begin(115200);
  WiFi.hostname(DEVICE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { 
  delay(500);
  Serial.print ( "." );
  }
  if(WL_CONNECTED == true){
  Serial.print("Successfull");
  }
  u8g2.drawStr(0,35, "2. -Start Alexa API");
  u8g2.sendBuffer();          // transfer internal memory to the display
  SetWebPages();
  // Define devices 
  // Types EspalexaDeviceType::onoff ,EspalexaDeviceType::dimmable, 127,EspalexaDeviceType::whitespectrum,EspalexaDeviceType::color
  espalexa.addDevice(AlexaDeviceName, StatusGuestWLAN, EspalexaDeviceType::dimmable);   
  espalexa.begin(&server);
  u8g2.drawStr(0,45, "1. -Start TR064");
  u8g2.sendBuffer();          // transfer internal memory to the display
  tr064_connection.init();
  u8g2.drawStr(0,55, "0. -FritzBox Connect");
  u8g2.sendBuffer();          // transfer internal memory to the display
  if (!(SwitchGuestWifi(false)))
    { 
    DisplayError("Dummy");
    }  else
    {
    Serial.println ( "First Authtication Success!" );
    }
  u8g2.clearBuffer();          // clear the internal memory 
  digitalWrite(LCD_BACKLIGHT_PIN, LOW);  
}




void GenerateQRCodefromText (char* Message,byte XStartPoint,String SSIDName, String SSIDPwd) // maximal 132 Zeichen QR Code !!!
{
  uint8_t qrcodeData[qrcode_getBufferSize(8)];
  int zeile = 2;
  int reihe = XStartPoint +2;
  qrcode_initText(&qrcode, qrcodeData, 8, 0, Message);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(10,10, "Guest WLAN");
  u8g2.setFont(u8g2_font_4x6_tf);
  String tmpSSIDstr = "ID: "; //+ SSIDName;
  int StrLngth = tmpSSIDstr.length();
  char ChrBuff[StrLngth];
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = tmpSSIDstr[a]; }
  u8g2.drawStr(0,20,ChrBuff);
  tmpSSIDstr = "Pwd: " + SSIDPwd;
  StrLngth = tmpSSIDstr.length();
  char ChrBuffB[StrLngth];
  for (int a = 0; a < StrLngth;a++){ ChrBuffB[a] = tmpSSIDstr[a]; }
  u8g2.drawStr(0,30,ChrBuffB);
  u8g2.drawStr(0,50,"Open 24h");
  u8g2.drawStr(0,60,"T-Remain: 01:10:10");
  //u8g2.drawStr(0,50,"01:10:30"); 
  //u8g2.setDrawColor(0);
//u8g2.drawBox(reihe-2, zeile-2, (qrcode.size *2) +4, (qrcode.size * 2) +4);
//u8g2.setDrawColor(1);
for (uint8_t y = 0; y < qrcode.size; y++) 
  {
    for (uint8_t x = 0; x < qrcode.size; x++) 
      {
      if(qrcode_getModule(&qrcode, x, y)) // Print solid Block
        {
         u8g2.drawBox(reihe, zeile, 1, 1); 
        } 
      reihe = reihe  + 1; 
      }

  reihe = XStartPoint +2;
  zeile = zeile + 1;     
  }
u8g2.drawFrame(XStartPoint, 0, qrcode.size+4, qrcode.size+15) ;  
u8g2.setFont(u8g2_font_7x13_tf);  // choose a suitable font
u8g2.drawStr(XStartPoint+ 2,qrcode.size+13,"Scan Me");  // write something to the internal memory
u8g2.sendBuffer();          // transfer internal memory to the display
}

void DisplayError (String Error) 
{
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    u8g2.drawStr(0,10, "Fehler!");
    u8g2.sendBuffer();          // transfer internal memory to the display  
}


void loop() 
{
 if (WLANStateHelp)
 {  
  Serial.println ( "Go in Main Loop Worker for WLAN Status." );
    if(WLANState) 
    {
    Serial.println ( "Worker: TURN ON WLAN" );  
    digitalWrite(LCD_BACKLIGHT_PIN, HIGH);   
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    u8g2.drawStr(0,10, "Zugangsdatengenerierung");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0,25,"Fritzbox wird konfiguriert");
    u8g2.drawStr(0,35,"Bitte warten..");  
    u8g2.sendBuffer();          // transfer internal memory to the display  
    String GSSID = GenerateSSID();
    String GWANPW = GenerateWLANPassword();
    String ClTextQRCode = "WIFI:T:WPA;S:"+ GSSID + ";P:" + GWANPW + ";H:;;"; // WLAN Credentials 
    int StrLngth = ClTextQRCode.length();
    char ChrBuff[StrLngth];
    bool ErrorOccured ;
    ErrorOccured = false;
    u8g2.drawBox(20, 40, 4, 4);    
    u8g2.sendBuffer();          // transfer internal memory to the display   
    if (!(NewSSIDGuestWifi(GSSID))) { ErrorOccured = true; }   
    u8g2.drawBox(30, 40, 4, 4); 
    u8g2.sendBuffer();          // transfer internal memory to the display  
    if (!(SetPWGuestWifi(GWANPW))) { ErrorOccured = true; } 
    u8g2.drawBox(40, 40, 4, 4);  
    u8g2.sendBuffer();          // transfer internal memory to the display  
    if (!(SwitchGuestWifi(true))) { ErrorOccured = true; } 
    u8g2.drawBox(50, 40, 4, 4);  
    u8g2.sendBuffer();          // transfer internal memory to the display
    u8g2.drawBox(60, 40, 4, 4); 
    delay(1000);  
    u8g2.clearBuffer();          // clear the internal memory
    if (ErrorOccured)
      { 
      DisplayError("Fritzbox Kommunikation");
      } else
      {       
      for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = ClTextQRCode[a]; }
      //GenerateQRCodefromText("WIFI:T:WPA;S:S;P:Password;H:;;",75); // WLAN Credentials 
      GenerateQRCodefromText(ChrBuff,75,GSSID,GWANPW); // WLAN Credentials
      }   
    } else
    {   
    Serial.println ( "Worker: TURN OFF WLAN" );    
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    u8g2.drawStr(0,10, "Ausschalten...");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0,25, "Bitte warten..");
    u8g2.sendBuffer();          // transfer internal memory to the display
    if (!(SwitchGuestWifi(false)))
      { 
      DisplayError("Fritzbox Kommunikation"); 
      } else
      {
      digitalWrite(LCD_BACKLIGHT_PIN, LOW);
      u8g2.clearBuffer();          // clear the internal memory
      u8g2.sendBuffer();          // transfer internal memory to the display
      }
    }
 WLANStateHelp = false;
 }

 espalexa.loop();
 delay(1);
}
