//#define OTA       // OTA is enabled

#define TR64ENABLED   // TR64 is enabled   
#define WiFiManagerENABLED  // WiFiManager is ENABLED
#define WDT_TIMEOUT         10  // 10 seconds WDT Timeout

#include <Arduino.h>
#include <U8g2lib.h>
#include "qrcode.h"  // Functions to generate a QRCode
#include <esp_task_wdt.h>
#define ESPALEXA_ASYNC //it is important to define this before #include <Espalexa.h>!
#include <Espalexa.h>  
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

const char OTAPASSWORD[] = "MyOTAPassword";           // Password for OTA

// ESP 32 IO Definitions

#define LCD_BACKLIGHT_PIN     4    // Output Pin for Backlight
#define SETUP_BUTTON_PIN      17   // Input Pin for GeneralControl & WiFiReset 
#define EXT_BUTTON_PIN        14   // Input Pin for WLAN Control 

#define LCD_Display_E_PIN     18   // Data to LCD - Enable Pin on LCD
#define LCD_Display_RW_PIN    23   // LCD R/W Pin 
#define LCD_Display_RS_PIN    5    // LCD RS Pin 
#define LCD_Display_Reset_PIN 22   // LCD RST Pin


#define AlexaDeviceName "GUESTWLAN_Switch"
#define ESPALEXA_MAXDEVICES 2      // set maximum devices add-able to Espalexa

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

// Global Variables for Basic Functions
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long WiFiWatchdogpreviousMillis = 0;
bool ButtonPressed = false;
bool debug = false;
bool WLANState = false;
bool WLANStateHelp = true;
bool Tr64ConfigAvail = false;
String OLDGenSSID = "";
// interrupt Control
bool SecInterruptOccured = false;
hw_timer_t * timer = NULL;
hw_timer_t * wdtimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// Clock Variables
int CntDwnSeconds = 0;
int CntDwnMinutes = 0;
int CntDwnHours = 0;
byte HourToCloseWlan = 3;  //Stunde,zu dieser das WLAN automatisch beendet wird.
byte WifiPasswordlenght = 9;
String StrLocalIPAddr = "";

#include "BasicFunctions.h"
#include "Webpages.h"
#include "TR64.h"


void GenerateQRCodefromText (char* Message,byte XStartPoint,byte YStartPoint,byte qrcodecapacity, byte qrcodeErrorTolerance ) // maximal 132 Zeichen QR Code !!!
{
  uint8_t qrcodeData[qrcode_getBufferSize(8)];
  int zeile = YStartPoint +2;
  int reihe = XStartPoint +2; 
  qrcode_initText(&qrcode, qrcodeData, qrcodecapacity, qrcodeErrorTolerance, Message); // maximal Stufe 2 Error Correction
  u8g2.setDrawColor(0);                                          // Clear Screen for QR Code
  u8g2.drawBox(XStartPoint, YStartPoint, qrcode.size+3, qrcode.size+3) ;  // Clear Screen for QR Code
  u8g2.setDrawColor(1);                                          // Clear Screen for QR Code
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
  u8g2.drawFrame(XStartPoint,  YStartPoint, qrcode.size+4, qrcode.size+4) ;
  u8g2.drawFrame(XStartPoint,YStartPoint +qrcode.size+3, qrcode.size+4,15) ;   
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(XStartPoint+ 3, YStartPoint + qrcode.size+13,"QRCode");  
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void QRCodeURL (String URL)
{
  String ClTextQRCode = URL;
  int StrLngth = ClTextQRCode.length()+1;
  char ChrBuff[StrLngth];
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = 0; } 
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = ClTextQRCode[a]; }
  GenerateQRCodefromText(ChrBuff,85,2,5, 2); // URL in QR Code Form 
}

void QRCodeGuestWLANInfo (String GSSID, String GWANPW, String Authentication)
{
  // Authentication:
  //  nopass - Keine Authentifikation
  //  WEP - WEP Authentifikation
  //  WPA - WPA/WPA2 Authentifikation
  String ClTextQRCode = "WIFI:T:" + Authentication + ";S:"+ GSSID + ";P:" + GWANPW + ";H:;;"; // WLAN Credentials TextCode
  // OLD:  String ClTextQRCode = "WIFI:T:WPA;S:"+ GSSID + ";P:" + GWANPW + ";H:;;"; // WLAN Credentials TextCode
  int StrLngth = ClTextQRCode.length()+1;
  char ChrBuff[StrLngth];
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = 0; } 
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = ClTextQRCode[a]; }
  GenerateQRCodefromText(ChrBuff,85,2,5, 2); // WLAN Credentials in QR Code Form 
}
// VCard Tempate
//BEGIN:VCARD
//VERSION:2.1
//FN:Max Mustermann
//N:Mustermann;Max
//TITLE:Dr.-Ing.
//TEL;CELL:+49 163 1737743
//TEL;WORK;VOICE:+49 123 7654321
//TEL;HOME;VOICE:+49 123 123456
//EMAIL;HOME;INTERNET:max.mustermann@example.com
//EMAIL;WORK;INTERNET:work@example.org
//URL:http://example.com
//ADR:;;Musterstraße 123;Musterstadt;;77777;Deutschland
//ORG:ACME Inc.
//END:VCARD



void Show_Error_onDisplay (String Error) 
{
  int StrLngth = 0;
  StrLngth = Error.length();
  char ChrBuff[StrLngth+2];
  Error.toCharArray(ChrBuff, StrLngth+1);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(0,10, "Fehler!");
  if (StrLngth < 22) { u8g2.setFont(u8g2_font_squeezed_b7_tr); } else { u8g2.setFont(u8g2_font_4x6_tf); } 
  u8g2.drawStr(0,30,ChrBuff);
  u8g2.sendBuffer();          // transfer internal memory to the display
  mydelay(5000);
}


void Show_TR64_Setup_QRCodeonDisplay()
{
int StrLngth = 0;
int a = 0;
  u8g2.clearBuffer();          // clear the internal memory      
  StrLocalIPAddr = WiFi.localIP().toString().c_str();
  String TR64ConfigURL = "http://" +StrLocalIPAddr + "/tr64";
  QRCodeURL (TR64ConfigURL);  
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(5,10, "TR64 Einstellung");
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,25, "Konfigurationsaufruf");   
  TR64ConfigURL = "http://" +StrLocalIPAddr;
  StrLngth = TR64ConfigURL.length();
  char ChrBuff[StrLngth+2];
  TR64ConfigURL.toCharArray(ChrBuff,StrLngth+1);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,35,ChrBuff);; 
  u8g2.drawStr(0,45, "oder QR-Code scannen"); 
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  u8g2.sendBuffer();          // transfer internal memory to the display
  do   // infinity LOOP
    {
    espalexa.loop();
    esp_task_wdt_reset();
    yield();
    } while (true); 
  
}

bool Initalize_TR64()
{
String tmpHrStr = "";
char   ChrBuff[32]= "                               ";
String TR64ConfigA ="";
String TR64ConfigB ="";
String TR64ConfigC ="";
String TR64ConfigD ="";
String TR64ConfigE ="";
  if(!SPIFFS.begin(true))
    {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
    }
  if(debug) {listAllFiles(); }  
  if(SPIFFS.exists("/tr64-config"))
    {
    Serial.println("TR64 Config exists");
    File file2 = SPIFFS.open("/tr64-config",FILE_READ);
    TR64ConfigA=file2.readStringUntil('\n');
    TR64ConfigB=file2.readStringUntil('\n');
    TR64ConfigC=file2.readStringUntil('\n');
    TR64ConfigD=file2.readStringUntil('\n');
    file2.close();   
    char TR64_User[TR64ConfigA.length()];
    TR64ConfigA.toCharArray(TR64_User, TR64ConfigA.length());
    char TR64_Password[TR64ConfigB.length()];
    TR64ConfigB.toCharArray(TR64_Password, TR64ConfigB.length());
    char TR64_IP[TR64ConfigC.length()];  
    TR64ConfigC.toCharArray(TR64_IP, TR64ConfigC.length());
    int TR64_PORT = TR64ConfigD.toInt();   
    Serial.println(TR64_User);
    Serial.println(TR64_Password);
    Serial.println(TR64_IP);
    Serial.println(TR64_PORT);
    Serial.print("Starte TR064");
    u8g2.drawStr(0,45, "1. -Starte TR064");    
    u8g2.sendBuffer();          // transfer internal memory to the display
    tr064_connection.setServer(TR64_PORT, TR64_IP, TR64_User, TR64_Password);
    tr064_connection.init();
    return true;
    } else { return false; }
return true;
}


void setup() 
{
  u8g2.begin(); 
  delay(200);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);    // sets the LCD_BACKLIGHT_PIN as output
  pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(EXT_BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(0,10, "Booting...");
  u8g2.sendBuffer();          // transfer internal memory to the display
  u8g2.setFont(u8g2_font_4x6_tf);
  WLANState  = false;
  WLANStateHelp = true;
  u8g2.drawStr(0,25, "3. -Starte WiFi");
  u8g2.sendBuffer();          // transfer internal memory to the display
  Serial.begin(115200);
  WiFi.hostname(DEVICE_NAME); 
#ifdef WiFiManagerENABLED
  SPIFFS.begin(true);  // Will format on the first run after failing to mount   
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
         u8g2.drawStr(60,25, "(Debug Modus an)");
         u8g2.sendBuffer();          // transfer internal memory to the display
         Serial.println ("Serial Debug Mode: enabled"); 
         SWDebugMode = true;
        }
      if ((WiFiWatchdogpreviousMillis > 9000) && (!(SWClearWiFiSettings)))
        {
         Serial.println ("Clearing WiFi Settings.. done.");
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
  u8g2.drawStr(60,25, "(WLAN OK)   ");
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
    espalexa.loop();
    esp_task_wdt_reset();
    yield();
    } while (true);
  };
  WiFiSettings.onWaitLoop = []() {
  Serial.print ("."); 
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
  AsyncElegantOTA.begin(&server,"admin",OTAPASSWORD);    // Start AsyncElegantOTA
#endif
  espalexa.begin(&server);
  SetRTCClock();
  // Define devices 
  // Types EspalexaDeviceType::onoff ,EspalexaDeviceType::dimmable, 127,EspalexaDeviceType::whitespectrum,EspalexaDeviceType::color
  espalexa.addDevice(AlexaDeviceName, StatusGuestWLAN, EspalexaDeviceType::dimmable);   
  // esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  // esp_task_wdt_add(NULL); //add current thread to WDT watch
  timer = timerBegin(0, 80, true); // Start Clock //timer 0, div 80
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer); 
#ifdef TR64ENABLED 
  if (Initalize_TR64())
    { 
      Serial.println ( "TR64 Konfiguration vorhanden" );
      Tr64ConfigAvail = true;
      u8g2.drawStr(0,55, "0. -FritzBox Verbindung");
      u8g2.sendBuffer();          // transfer internal memory to the display
      if (!(SwitchGuestWifi(false))) 
        { 
          Serial.println ( "Fehlerhafte TR64 Konfiguration" );
          Show_Error_onDisplay ("Fehler in TR64 Config");
          Tr64ConfigAvail = false;
          delay(9000);
          SPIFFS.remove("/tr64-config");
          Show_TR64_Setup_QRCodeonDisplay();
        }   
    } else 
    { 
      Serial.println ( "Keine TR64 Konfiguration vorhanden" );
      Tr64ConfigAvail = false;
      Show_Error_onDisplay ("Keine TR64 Config"); 
      delay(9000);
      Show_TR64_Setup_QRCodeonDisplay();    
    }
#endif 
  u8g2.clearBuffer();          // clear the internal memory        
}


void Show_Timers()
{
  String tmpCntDwnStr = "";
  String tmpSecStr = "";
  String tmpMinStr = "";
  String tmpHrStr = "";
  char   ChrBuff[23]= "                      ";
  String Realtime = rtc.getTime();
  int StrLngth = 0;
  int a = 0;
  if (CntDwnSeconds > 59) { CntDwnSeconds = 59;}
  if (CntDwnSeconds < 0) { CntDwnSeconds = 0;}
  tmpSecStr = String(CntDwnSeconds);
  if ((CntDwnSeconds < 10) & (CntDwnSeconds  >0)) { tmpSecStr = "0" + String(CntDwnSeconds); } else { tmpSecStr = String(CntDwnSeconds); }
  if (CntDwnSeconds == 0) { tmpSecStr = "00";} 
  if ((CntDwnMinutes < 10) & (CntDwnMinutes  >0))  { tmpMinStr = "0" + String(CntDwnMinutes); } else { tmpMinStr = String(CntDwnMinutes); }  
  if (CntDwnMinutes == 0) { tmpMinStr = "00";} 
  if ((CntDwnHours < 10) & (CntDwnHours  >0))  { tmpHrStr = "0" + String(CntDwnHours); } else { tmpHrStr = String(CntDwnHours); } 
  if (CntDwnHours == 0) { tmpHrStr = "00";}
  tmpCntDwnStr = "Uhrzeit:  " + rtc.getTime();
  StrLngth = tmpCntDwnStr.length();
  for (a = 0; a < StrLngth;a++){ ChrBuff[a] = tmpCntDwnStr[a]; }
  ChrBuff[StrLngth+3] = char(0);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,50,ChrBuff); // Alternative:String.c_str()
  tmpCntDwnStr = "Restzeit: " + tmpHrStr + ":" + tmpMinStr + ":" + tmpSecStr;
  StrLngth = tmpCntDwnStr.length();
  for (a = 0; a < StrLngth;a++){ ChrBuff[a] = tmpCntDwnStr[a]; }
  ChrBuff[StrLngth+1] = char(10);
  ChrBuff[StrLngth+2] = char(13);
  ChrBuff[StrLngth+3] = char(0);
  u8g2.drawStr(0,60,ChrBuff);
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void ClearTextGuestWLANInfo ( String SSIDName, String SSIDPwd, char ChrTitle[13])
{
  int a = 0;
  int StrLngth = 0;
  String tmpSSIDstr = "";
  char   ChrBuff[23]= "                      ";
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  //u8g2.drawStr(7,10, "Gaeste WLAN");
  u8g2.drawStr(7,10, ChrTitle);
  u8g2.setFont(u8g2_font_4x6_tf);
  tmpSSIDstr = "SSID: " + SSIDName;
  StrLngth = tmpSSIDstr.length();
  for (a = 0; a < StrLngth;a++){ ChrBuff[a] = tmpSSIDstr[a]; }
  ChrBuff[StrLngth+1] = char(0);
  u8g2.drawStr(0,20,ChrBuff);
  for (a = 0; a < 23;a++){ ChrBuff[a] = 0; }
  tmpSSIDstr = "Pwrt: " + SSIDPwd;
  StrLngth = tmpSSIDstr.length();
  for (a = 0; a < StrLngth;a++){ ChrBuff[a] = tmpSSIDstr[a]; }
  ChrBuff[StrLngth+1] = char(0);
  u8g2.drawStr(0,30,ChrBuff);
  u8g2.sendBuffer();          // transfer internal memory to the display
}


void DisableGuestWlan()
{
    Serial.println ( "Worker: TURN OFF WLAN" );    
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
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.sendBuffer();          // transfer internal memory to the display
    digitalWrite(LCD_BACKLIGHT_PIN, LOW);
    WLANState = false;
}





void WLANSwitchWorker()
{
 if (WLANStateHelp)
 {  
  if (debug) { Serial.println ( "Go in Main Loop Worker for WLAN Status." ); }
    if(WLANState) 
    {
    Serial.println ( "Worker: TURN ON WLAN" );  
    digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
    u8g2.begin(); 
    delay(200); 
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    u8g2.drawStr(0,10, "Zugangsdatengenerierung");
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0,25,"Fritzbox wird konfiguriert");
    u8g2.drawStr(0,35,"Bitte warten..");  
    u8g2.sendBuffer();          // transfer internal memory to the display     
    String GSSID = GenerateSSID(true);   
    String GWANPW = GenerateWLANPassword(WifiPasswordlenght+1);
    bool ErrorOccured =false;
    u8g2.drawBox(20, 40, 4, 4);    
    u8g2.sendBuffer();          // transfer internal memory to the display
    #ifdef TR64ENABLED 
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
    #endif
    delay(1000);  
    u8g2.clearBuffer();          // clear the internal memory
    if (ErrorOccured)
      { 
      Show_Error_onDisplay ("Fritzbox Kommunikation");
      Serial.println ( "Fehler in Fritzbox Kommunikation" );
      WLANState = false;
      } else
      {       
      CntDwnSeconds = 59 - rtc.getSecond();
      CntDwnMinutes = 59 - rtc.getMinute();     
      int temp = (23 -rtc.getHour(true)) + HourToCloseWlan; // 
      if (temp > 23){ temp = temp - 24; }
      CntDwnHours = temp;
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


void TimerServices()
{
if (SecInterruptOccured)
  {
  SecInterruptOccured = false; 
  if (WLANState)
    {
    if ((CntDwnSeconds == 0 ) & (CntDwnMinutes == 0) & (CntDwnHours == 0))  
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
        Serial.println ("Hardware Button Level 1 Pressed" );  
        WLANState  = !(WLANState);
        WLANStateHelp = true;
        ButtonPressed = true;
        }
       WLANSwitchWorker();
       TimerServices();  
       espalexa.loop();
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
  } 
 espalexa.loop();
 esp_task_wdt_reset();
 yield();
}
