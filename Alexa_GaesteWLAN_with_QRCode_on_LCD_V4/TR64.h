#ifdef TR64ENABLED 

#include <tr064.h>

// const char FRITZBOX_IP[] = "111.111.111.111";
// const int FRITZBOX_PORT = 4900
// const char USER[] = "MyTr64User";
// const char PASSWORD[] = "MyTr64User"; // <-- ändern
// TR064 tr064_connection(FRITZBOX_PORT, FRITZBOX_IP, USER, PASSWORD);

TR064 tr064_connection;

bool SwitchGuestWifi(bool status) 
{
String params[][2] = {{"NewEnable", "0"}};
String req[][2] = {{}};

yield();
esp_task_wdt_reset();
  if (status) 
    {
    params[0][1] = "1";
    if (debug) { Serial.println ("Set WLAN status: on"); }
    } else {
    if (debug) { Serial.println ("Set WLAN status: off"); }
    }
  if (tr064_connection.action("urn:dslforum-org:service:WLANConfiguration:3", "SetEnable", params, 1, req, 2))
    {
    esp_task_wdt_reset();
    return true;
    } else
    {
    esp_task_wdt_reset();  
    return false;
    } 
}

bool NewSSIDGuestWifi(String NewSSID) 
{
String params[][2] = {{"NewSSID",  NewSSID}};
String req[][2] = {{}};

yield();
esp_task_wdt_reset(); 
 
  if (tr064_connection.action("urn:dslforum-org:service:WLANConfiguration:3", "SetSSID", params, 2, req, 2))
    { 
    if (debug) { Serial.println ("SetNewSSID true"); }
    esp_task_wdt_reset();
    return true;
    } else
    {
    if (debug) { Serial.println ("SetNewSSID false"); }
    esp_task_wdt_reset();
    return false;
    }
}

bool SetPWGuestWifi(String StrPassword) 
{   
String params[6][2] = {{ "NewWEPKey0", "" }, { "NewWEPKey1", "" }, { "NewWEPKey2", ""  }, { "NewWEPKey3", ""  }, { "NewPreSharedKey", "A487C40982581DB9F71090303754A53E9015409AA2489697F83278765257F01C" }, { "NewKeyPassphrase", StrPassword }};
String req[6][2] = {{}};

yield();
esp_task_wdt_reset();  

  if (tr064_connection.action("urn:dslforum-org:service:WLANConfiguration:3", "SetSecurityKeys", params, 6, req, 2)) 
    { 
    if (debug) { Serial.println ("SetPWGuestWifi true"); }
    esp_task_wdt_reset();
    return true;
    } else
    {
    if (debug) { Serial.println ("SetPWGuestWifi false"); }
    esp_task_wdt_reset();
    return false;
    }
}

void Dialing() 
{
String params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**799"}};
String req[][2] = {{}};

yield();
esp_task_wdt_reset();
  
  tr064_connection.action("X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0);
  //connection.action("urn:dslforum-org:service:X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0); 
  // without loading available services through init() you have to set the url
  //connection.action("X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0, "/upnp/control/x_voip");
esp_task_wdt_reset();
}


void Show_TR64_Setup_QRCodeonDisplay()
{
  esp_task_wdt_reset();
  u8g2.clearBuffer();          // clear the internal memory      
  StrLocalIPAddr = WiFi.localIP().toString().c_str();
  String TR64ConfigURL = "http://" +StrLocalIPAddr + "/tr64";
  QRCodeURL (TR64ConfigURL);  
  u8g2.setFont(u8g2_font_squeezed_b7_tr);
  u8g2.drawStr(5,10, "TR64 Einstellung");
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,25, "Konfigurationsaufruf");   
  TR64ConfigURL = "http://" +StrLocalIPAddr;
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0,35,TR64ConfigURL.c_str()); 
  u8g2.drawStr(0,45, "oder QR-Code scannen"); 
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  u8g2.sendBuffer();          // transfer internal memory to the display
  do   // infinity LOOP
    {
#ifdef VOICEASSISTANT      
    espalexa.loop();
#endif
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

yield();
esp_task_wdt_reset();

  if(!SPIFFS.begin(true))
    {
    if (debug) { Serial.println("An Error has occurred while mounting SPIFFS"); }
    return false;
    }
  if(debug) {listAllFiles(); }  
  if(SPIFFS.exists("/tr64-config"))
    {
    if (debug) { Serial.println("TR64 Config exists"); }
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
    yield();
    esp_task_wdt_reset();  
    if (debug) 
      {
      Serial.print("TR64 Verbindungsparamenter:");  
      Serial.println(TR64_User);
      Serial.println("TR64_Password");
      Serial.println(TR64_IP);
      Serial.println(TR64_PORT);
      Serial.print("Starte TR064");
      }
    u8g2.drawStr(0,45, "1. -Starte TR064");    
    u8g2.sendBuffer();          // transfer internal memory to the display
    tr064_connection.setServer(TR64_PORT, TR64_IP, TR64_User, TR64_Password);
    tr064_connection.init();
    yield();
    esp_task_wdt_reset();
    return true;
    } else { return false; }
yield();
esp_task_wdt_reset();
return true;
}



#endif
