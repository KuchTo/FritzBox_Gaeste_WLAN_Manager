// This is a Project for a comfortable Guest WLAN Control in Ceonjunction with a Fritz Box that allows control of the Network State via TR 64 Protocol
// You can enable or disable extra Functions like Alexa Voice Control, OTA and a WiFi Manager
/**
 *
 * Copyright (c) 2023 Tobias Kuch */
 

#define OTA                    // Uncomment if OTA should be enabled Need <AsyncElegantOTA.h> https://github.com/ayushsharma82/ElegantOTA
#define TR64ENABLED            // Uncomment if TR64 should be enabled 
#define VOICEASSISTANT         // Uncomment if Alexa Voiceassistant should be enabled Need <Espalexa.h> https://github.com/Aircoookie/Espalexa
#define WiFiManagerENABLED     // Uncomment if WiFiManager should be enabled Note: WiFiSettings and SPIFFS Libarys needed ! https://github.com/tzapu/WiFiManager 

#define WDT_TIMEOUT_SECONDS 30 // 30 seconds WDT Timeout
#define CheckWLANState_SEC 30 // Checks for an Active and alive WLAN Connection Watchdog

#include <Arduino.h>
#include <U8g2lib.h>
#include "qrcode.h"  // Functions to generate a QRCode
#include <esp_task_wdt.h>
#ifdef VOICEASSISTANT
  #define ESPALEXA_ASYNC //it is important to define this before #include <Espalexa.h>!
  #include <Espalexa.h>
#else
  #include <ESPAsyncWebServer.h>
#endif   
  #include <FS.h>              
#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESP32Time.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif

#ifdef WiFiManagerENABLED
  #include <WiFiSettings.h>
  #include <SPIFFS.h>
#endif

#include <esp_task_wdt.h>
#include <EasyNTPClient.h>

#ifndef WiFiManagerENABLED 
  const char WIFI_SSID[] = "MyWLANSSID";          // <-- Change this !
  const char WIFI_PASSWORD[] = "MyWLANPassword";  // <-- Change this !
#endif

const char DEVICE_NAME[] = "GuestWLAN_Sw";
const char OTAUSER[] = "admin";           // USER for OTA
const char OTAPASSWORD[] = "myOTAPassword";           // Password for OTA

// setting PWM properties for LCD Backlight
const int ledfreq = 5000;
const int LcdBacklightCh = 0;
const int ledresolution = 8;

struct SysConfig
  {
   int Seconds = 0;
   int Minutes = 0;
   int Hours = 0;
   int Days = 0;
   String OTA_User = "admin";
   String OTA_Password = "myOTAPassword";    
  };

// ESP 32 IO Definitions

#define LCD_BACKLIGHT_PIN     4    // Output Pin for Backlight
#define SETUP_BUTTON_PIN      17   // Input Pin for GeneralControl & WiFiReset 
#define EXT_BUTTON_PIN        14   // Input Pin for WLAN Control 

#define LCD_Display_E_PIN     18   // Data to LCD - Enable Pin on LCD
#define LCD_Display_RW_PIN    23   // LCD R/W Pin 
#define LCD_Display_RS_PIN    5    // LCD RS Pin 
#define LCD_Display_Reset_PIN 22   // LCD RST Pin

#ifdef VOICEASSISTANT
  #define AlexaDeviceName1 "GUESTWLAN_Switch"
  #define AlexaDeviceName2 "GUESTWLAN_Switch_BackLight"
  #define ESPALEXA_MAXDEVICES 3      // set maximum devices add-able to Espalexa
#endif

#ifdef U8X8_HAVE_HW_SPI
  #include <SPI.h>
#endif
  #ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#ifdef OTA
  #include <AsyncElegantOTA.h>
#endif

ESP32Time rtc(3600);  // offset in seconds GMT+1
 
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0,LCD_Display_E_PIN,LCD_Display_RW_PIN,LCD_Display_RS_PIN,LCD_Display_Reset_PIN ); // ESP32
//U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* E(data)=*/ 18, /* R/W=*/ 23, /* RS=*/ 5, /* reset=*/ 22); // ESP32

QRCode qrcode;

SysConfig MyConfig;

// Global Variables for Basic Functions
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long WiFiWatchdogpreviousMillis = 0;
unsigned long ChWLStateSecCounter = 0;

bool ButtonPressed = false;
bool debug = false;
bool WLANState = false;
bool WLANStateHelp = true;
bool Tr64ConfigAvail = false;
String OLDGenSSID = "";
String GSSID = ""; 
String GWANPW = "";


// interrupt Control
bool SecInterruptOccured = false;
hw_timer_t * timer = NULL;
hw_timer_t * wdtimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// Clock Variables
int CntDwnSeconds = 0;
int CntDwnMinutes = 0;
int CntDwnHours = 0;
int CntDwnDays = 0;
byte currentLEDBrightness = 0;
byte wantedLEDBrightness = 0;

String StrLocalIPAddr = "";

#include "BasicFunctions.h"
#include "Webpages.h"
#include "TR64.h"


void setup() 
{
  u8g2.begin(); 
  delay(200);
  ledcSetup(LcdBacklightCh,ledfreq,ledresolution);
  ledcAttachPin(LCD_BACKLIGHT_PIN, LcdBacklightCh); // attach the channel to the GPIO to be controlled
  pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(EXT_BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  SetBackLightBrightness(100);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(0,10, "Booting...");
  u8g2.sendBuffer();          // transfer internal memory to the display
  u8g2.setFont(u8g2_font_4x6_tf); 
  WLANState  = false;
  WLANStateHelp = true;
  u8g2.drawStr(0,25, "3. -Init WiFi");
  u8g2.sendBuffer();          // transfer internal memory to the display
  WiFi.hostname(DEVICE_NAME); 
  SPIFFS.begin(true);  // Will format on the first run after failing to mount
  bool result = ConfigtoRead(MyConfig);
  if (!(result))
    {
    MyConfig.Hours = 8;
    }
#ifdef WiFiManagerENABLED  
  if (!(digitalRead(SETUP_BUTTON_PIN)))
    {
    bool SWDebugMode = false;
    bool SWClearWiFiSettings = false;
    previousMillis = millis();
    currentMillis = millis();
    do
      {
      currentMillis = millis();  
      WiFiWatchdogpreviousMillis = currentMillis - previousMillis;
      if ((WiFiWatchdogpreviousMillis > 500) && (!(SWDebugMode)))
        {
         //Serial.begin(115200); 
         debug = true;
         u8g2.drawStr(60,25, " (Debug Modus an)");
         u8g2.sendBuffer();          // transfer internal memory to the display
         Serial.println ("Serial Debug Mode: enabled"); 
         SWDebugMode = true;
        }
      if ((WiFiWatchdogpreviousMillis > 9000) && (!(SWClearWiFiSettings)))
        {
         if (debug) { Serial.println ("Clearing WiFi Settings.. done."); }
         u8g2.drawStr(0,35, "!. -Factory Reset in 5 Seconds");
         u8g2.sendBuffer();          // transfer internal memory to the display
         mydelay(5000);
         SPIFFS.format();           
         SWClearWiFiSettings = true;
         mydelay(500);
         ESP.restart();
        }    
      } while (!(digitalRead(SETUP_BUTTON_PIN)));       
    previousMillis = 0;
    currentMillis = 0;
    WiFiWatchdogpreviousMillis = 0;
    }    
  WiFiSettings.onSuccess  = []() {
  if (debug) { Serial.println ("WLAN OK."); }
  String Iip = " (WLAN OK)";
  Iip =  "("+ WiFi.localIP().toString() + ")";
  u8g2.drawStr(55,25,Iip.c_str());
  u8g2.sendBuffer();          // transfer internal memory to the display
  };
  WiFiSettings.onFailure  = []() {
  if (debug) { Serial.println ("WLAN Fehler"); }
  u8g2.drawStr(60,25, " (WLAN Fehler)");
  u8g2.sendBuffer();          // transfer internal memory to the display
  delay(3000);
  Show_Error_onDisplay ("Keine WLAN Verbindung moeglich.");
  do   // infinity LOOP
    {
#ifdef VOICEASSISTANT  
    espalexa.loop();
#endif
    esp_task_wdt_reset();
    yield();
    } while (true);
  };
  WiFiSettings.onWaitLoop = []() {
  if (debug) { Serial.print ("."); }
  return 500; // mydelay next function call by 500ms
  };
  //WiFiSettings.onPortalWaitLoop = []() {
  //
  //return 500; // mydelay next function call by 500ms
  //};
  WiFiSettings.onPortalView = []() {
  if (debug) { Serial.println ("Captive Portal viewed."); }
  return 500; // mydelay next function call by 500ms
  };
  WiFiSettings.onPortal = []() {
  if (debug) { Serial.println ("Captive Portal started."); }
  u8g2.drawStr(60,25, " (Cap. Prt. AP)");
  u8g2.sendBuffer();
  delay(3000);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(0,10, "Ersteinrichtung");
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,25, "Einrichtung mit WLAN:");
  String CPSSID = "SSID: " + WiFiSettings.hostname;
  u8g2.drawStr(0,35, CPSSID.c_str());
  String CPPASS = "PW: " + WiFiSettings.password;
  u8g2.drawStr(0,45, CPPASS.c_str());
  u8g2.drawStr(0,55, "oder QR-Code scannen");
  u8g2.sendBuffer();
  QRCodeGuestWLANInfo (WiFiSettings.hostname.c_str(),WiFiSettings.password.c_str(),"WPA");
  return 500; // mydelay next function call by 500ms
  };
  WiFiSettings.onConfigSaved = []() {
  if (debug) { Serial.println ("Configuration in Captive Portal saved."); } 
  delay(500);
  ESP.restart();
  return 500; // mydelay next function call by 500ms
  };
  // Connect to WiFi with a timeout of 40 seconds
  // For Security Reasons: NOT Launches the portal if the connection failed
  u8g2.sendBuffer();
  WiFi.hostname(DEVICE_NAME);
  WiFiSettings.secure = true;
  WiFiSettings.connect(false, 40);    
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { 
  delay(500);
   if (debug) { Serial.print ( "." ); }
  }
  if(WL_CONNECTED == true){
  if (debug) { Serial.print("Successful"); }
  }  
#endif
  u8g2.drawStr(0,35, "2. -Starte Alexa API");  
  u8g2.sendBuffer();          // transfer internal memory to the display  
  SetWebPages();
#ifdef OTA
  AsyncElegantOTA.begin(&server,OTAUSER,OTAPASSWORD);    // Start AsyncElegantOTA
#endif
#ifdef VOICEASSISTANT
  espalexa.begin(&server);
#endif
  SetRTCClock(); 
#ifdef VOICEASSISTANT
  // Define devices 
  // Types EspalexaDeviceType::onoff ,EspalexaDeviceType::dimmable, 127,EspalexaDeviceType::whitespectrum,EspalexaDeviceType::color
  espalexa.addDevice(AlexaDeviceName1, StatusGuestWLAN, EspalexaDeviceType::dimmable);    
  espalexa.addDevice(AlexaDeviceName2,BackLightBrightness, EspalexaDeviceType::dimmable);
#endif
  timer = timerBegin(0, 80, true); // Start Clock //timer 0, div 80
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);   
  
#ifdef TR64ENABLED 
  if (Initalize_TR64())
    { 
      if (debug) { Serial.println ( "TR64 Configuration found" ); }
      Tr64ConfigAvail = true;
      u8g2.drawStr(0,55, "0. -FritzBox Verbindung");
      u8g2.sendBuffer();          // transfer internal memory to the display
      if (!(SwitchGuestWifi(false))) 
        { 
          if (debug) { Serial.println ( "Faulty TR64 Configuration found" ); }
          Show_Error_onDisplay ("Fehler in TR64 Config");
          Tr64ConfigAvail = false;
          mydelay(9000);
          SPIFFS.remove("/tr64-config");
          Show_TR64_Setup_QRCodeonDisplay();
        }   
    } else 
    { 
      if (debug) { Serial.println ( "No TR64 Configuration found") ; }
      Tr64ConfigAvail = false;
      Show_Error_onDisplay ("Keine TR64 Config"); 
      mydelay(9000);
      Show_TR64_Setup_QRCodeonDisplay();    
    }
#endif
#ifndef TR64ENABLED 
Tr64ConfigAvail = true;
u8g2.drawStr(0,45, "1. -TR064 deaktiviert");  
u8g2.drawStr(0,55, "0. -FritzBox Verbindung deaktiviert");
u8g2.sendBuffer();          // transfer internal memory to the display
delay(1000);
#endif
  esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true); //enable WATCHDOG
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  u8g2.clearBuffer();          // clear the internal memory       
}


void Show_Timers()
{
  String tmpCntDwnStr = "";
  String tmpSecStr = "";
  String tmpMinStr = "";
  String tmpHrStr = "";
  String tmpDayStr = "";
  String Realtime = rtc.getTime();
  if (CntDwnSeconds > 59) { CntDwnSeconds = 59;}
  if (CntDwnSeconds < 0) { CntDwnSeconds = 0;}
  tmpSecStr = String(CntDwnSeconds);
  if ((CntDwnSeconds < 10) & (CntDwnSeconds  >0)) { tmpSecStr = "0" + String(CntDwnSeconds); } else { tmpSecStr = String(CntDwnSeconds); }
  if (CntDwnSeconds == 0) { tmpSecStr = "00";} 
  if ((CntDwnMinutes < 10) & (CntDwnMinutes  >0))  { tmpMinStr = "0" + String(CntDwnMinutes); } else { tmpMinStr = String(CntDwnMinutes); }  
  if (CntDwnMinutes == 0) { tmpMinStr = "00";} 
  if ((CntDwnHours < 10) & (CntDwnHours  >0))  { tmpHrStr = "0" + String(CntDwnHours); } else { tmpHrStr = String(CntDwnHours); } 
  if (CntDwnHours == 0) { tmpHrStr = "00";}
  tmpDayStr = String(CntDwnDays) + "T ";
  tmpCntDwnStr = "Uhrzeit:   " + rtc.getTime();
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,50,tmpCntDwnStr.c_str()); // Alternative:String.c_str()
  tmpCntDwnStr = "Ablauf: " + tmpDayStr + tmpHrStr + ":" + tmpMinStr + ":" + tmpSecStr;
  u8g2.drawStr(0,60,tmpCntDwnStr.c_str());
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void ClearTextGuestWLANInfo ( String SSIDName, String SSIDPwd, char ChrTitle[13])
{
  String tmpSSIDstr = "";
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  //u8g2.drawStr(7,10, "Gaeste WLAN");
  u8g2.drawStr(13,10, ChrTitle);
  u8g2.setFont(u8g2_font_4x6_tf);
  tmpSSIDstr = "SSID: " + SSIDName;  
  u8g2.drawStr(0,20,tmpSSIDstr.c_str());
  tmpSSIDstr = "Pwrt: " + SSIDPwd;
  u8g2.drawStr(0,30,tmpSSIDstr.c_str());
  u8g2.sendBuffer();          // transfer internal memory to the display
}


void DisableGuestWlan()
{
    if (debug) { Serial.println ( "Worker: TURN OFF WLAN" ); }
    GWANPW = "";
    GSSID = "";
    CntDwnSeconds = 1; 
    CntDwnMinutes = 0;
    CntDwnHours = 0;
    CntDwnDays = 0; 
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    u8g2.drawStr(0,10, "Ausschalten...");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0,25, "Bitte warten..");
    u8g2.sendBuffer();          // transfer internal memory to the display
    #ifdef TR64ENABLED 
    if (!(SwitchGuestWifi(false)))
      { 
      Show_Error_onDisplay ("TR64 Kommunikationsfehler"); 
      WLANState  = false;
      WLANStateHelp = false;
      } 
    #endif     
    //SetBackLightBrightness(0);   // old: digitalWrite(LCD_BACKLIGHT_PIN, LOW);
    WLANState = false;
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.sendBuffer();           // transfer internal memory to the display
    FadeBackLightBrightness(0);
}

int CalculateRestTime (String unit)
 {
  byte RestHours = 8;
  return RestHours;
 }

void WLANSwitchWorker()
{
 if (WLANStateHelp)
 {  
  if (debug) { Serial.println ( "Go in Main Loop Worker for WLAN Status." ); }
    if(WLANState) 
    {
    if (debug) { Serial.println ( "Worker: TURN ON WLAN" ); }
    SetBackLightBrightness(30);   // old: digitalWrite(LCD_BACKLIGHT_PIN, LOW);
    esp_task_wdt_reset();
    //FadeBackLightBrightness(100);
    u8g2.begin(); 
    delay(200); 
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    u8g2.drawStr(0,10, "Zugangsdatengenerierung");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0,25,"Fritzbox wird konfiguriert");
    u8g2.drawStr(0,35,"Bitte warten..");  
    u8g2.sendBuffer();          // transfer internal memory to the display     
    GSSID = GenerateSSID(true);   
    GWANPW = GenerateWLANPassword();
    bool ErrorOccured =false;
    u8g2.drawBox(20, 40, 4, 4);    
    u8g2.sendBuffer();          // transfer internal memory to the display
    #ifdef TR64ENABLED 
    esp_task_wdt_reset();
    if (!(NewSSIDGuestWifi(GSSID))) { ErrorOccured = true; }   
    esp_task_wdt_reset();
    u8g2.drawBox(30, 40, 4, 4); 
    u8g2.sendBuffer();          // transfer internal memory to the display  
    esp_task_wdt_reset();
    if (!(SetPWGuestWifi(GWANPW))) { ErrorOccured = true; } 
    esp_task_wdt_reset();
    u8g2.drawBox(40, 40, 4, 4);  
    u8g2.sendBuffer();          // transfer internal memory to the display  
    esp_task_wdt_reset();
    if (!(SwitchGuestWifi(true))) { ErrorOccured = true; } 
    esp_task_wdt_reset();
    u8g2.drawBox(50, 40, 4, 4);  
    u8g2.sendBuffer();          // transfer internal memory to the display
    u8g2.drawBox(60, 40, 4, 4);   
    #endif
    delay(100);  
    u8g2.clearBuffer();          // clear the internal memory
    FadeBackLightBrightness(100);  
    if (ErrorOccured)
      {
      Show_Error_onDisplay ("Fritzbox Kommunikation");
      if (debug) { Serial.println ( "Fehler in Fritzbox Kommunikation" ); }
      CntDwnSeconds = 1; 
      CntDwnMinutes = 0;
      CntDwnHours = 0;
      CntDwnDays = 0;   
      WLANState = false;
      GWANPW = "";
      GSSID = "";
      } else
      {       
      CntDwnSeconds = MyConfig.Seconds; // Associate
      CntDwnMinutes = MyConfig.Minutes;
      CntDwnHours = MyConfig.Hours;
      CntDwnDays = MyConfig.Days;
      ClearTextGuestWLANInfo (GSSID,GWANPW,"Gaeste WLAN"); // WLAN Credentials Textinfo in Clear Text 
      QRCodeGuestWLANInfo (GSSID,GWANPW,"WPA"); // WLAN Credentials in QR Code Form
      
      Show_Timers();    
      }   
    } else
    {   
     DisableGuestWlan();
    }
 WLANStateHelp = false;
 }
 
} 

void CheckWLANStatus()
{
  if (ChWLStateSecCounter == CheckWLANState_SEC ) // Check WLAN State 
    {
    ChWLStateSecCounter = 0;
    if (WiFi.status() != WL_CONNECTED) // != WL_CONNECTED)
      {
      ledcWrite(LcdBacklightCh, 50);
      Show_Error_onDisplay("WLAN Verbindung verloren. Bitte warten..");
      esp_task_wdt_reset();
      int loopcount = 0;
      do 
        {
        if ( loopcount == 2160) { ESP.restart(); } // ca 12 Stunden Timer. Nach 12 Stunden - > Reboot
        if (debug) 
          { 
          Serial.print("WLAN disconnected. Try to Reconnect - Loop: " );         
          Serial.println(loopcount);
          }
        WiFi.disconnect();
        delay(5000);
        esp_task_wdt_reset();
        yield();       
        WiFi.reconnect();
        delay(5000);
        esp_task_wdt_reset();
        yield();       
        loopcount++;
        } while(WiFi.status() != WL_CONNECTED); // != WL_CONNECTED)
      loopcount = 0;
      if (debug) { Serial.println ("WLAN reconnected" ); }
      ledcWrite(LcdBacklightCh, currentLEDBrightness);
      u8g2.clearBuffer();
      u8g2.sendBuffer(); 
      }
    }
}

void TimerServices()
{
if (SecInterruptOccured)
  {
  SecInterruptOccured = false;
  if (WLANState)
    {
    if ((CntDwnDays == 0) & (CntDwnSeconds == 0 ) & (CntDwnMinutes == 0) & (CntDwnHours == 0))  
      { 
      SetRTCClock();
      DisableGuestWlan();  } else  { Show_Timers(); }
    }  
  }
}

void HWButtonHandler()
{
 if (!(digitalRead(SETUP_BUTTON_PIN)))
    {  
    previousMillis = millis();
    currentMillis = millis();
    do
      {
      currentMillis = millis();  
      WiFiWatchdogpreviousMillis = currentMillis - previousMillis;
      if ((WiFiWatchdogpreviousMillis > 200) && (!(ButtonPressed)))
        {
        if (debug) { Serial.println ("Hardware Button Level 1 Pressed" ); }
        WLANState  = !(WLANState);
        WLANStateHelp = true;
        ButtonPressed = true;
        }
       WLANSwitchWorker();
       TimerServices();
#ifdef VOICEASSISTANT        
       espalexa.loop();
#endif      
      yield();
      esp_task_wdt_reset();  
      } while (!(digitalRead(SETUP_BUTTON_PIN)));       
    previousMillis = 0;
    currentMillis = 0;
    ButtonPressed =  false;
    WiFiWatchdogpreviousMillis = 0;
    }  
}

void loop() // Hauptschleife
{
 if (Tr64ConfigAvail) 
  { // Nur laufen, wenn TR64 Config vorhanden ist.
    WLANSwitchWorker();
    TimerServices();
    HWButtonHandler();
    CheckWLANStatus();
  }
#ifdef VOICEASSISTANT
 espalexa.loop();
#endif
 BackLightBrightnessWorker();
 esp_task_wdt_reset();
 yield();
 CheckWLANStatus();
}
