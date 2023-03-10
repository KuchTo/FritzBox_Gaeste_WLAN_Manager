#ifdef VOICEASSISTANT
  Espalexa espalexa;
#endif
AsyncWebServer server(80);

WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((1*60*60))); // IST = GMT + 1:00


void IRAM_ATTR onTimer()      
  {
 // portENTER_CRITICAL_ISR(&timerMux);
  //Calculate Time 24 Hour Format
  SecInterruptOccured = true;
  ChWLStateSecCounter++; // WLAN Connect Watchdog Counter 
  if (! ( (CntDwnSeconds < 1) & (CntDwnMinutes < 1) & (CntDwnHours  < 1) & (CntDwnDays < 1) ) ) // Stop Counting Down if Counter Reaches 0
    {
    CntDwnSeconds--;  
    if (CntDwnSeconds < 0)
      {
      CntDwnSeconds = 59;
      CntDwnMinutes--;
      }
    if (CntDwnMinutes < 0)
      {
      CntDwnMinutes = 59;
      CntDwnHours--;      
      }
    if (CntDwnHours < 0)
      {
      CntDwnHours = 23;
      CntDwnDays--;
      }  
    }   
 // portEXIT_CRITICAL_ISR(&timerMux);
}
//Interrupts ende

String convertToString(char* a)
{
    String s(a);
    return s;
}

String GenerateWLANPassword()
{
  String ReturnStr = "12345678900";
  for (int a = 0;a < 11;a++)
    {
    byte Sign = random(0,3);
    if(Sign == 0) { ReturnStr[a] = char(random(65,90)); } // BigChar
    if(Sign == 1) { ReturnStr[a] = char(random(97,122)); } // littleChar 
    if(Sign == 2) { ReturnStr[a] = char(random(48,57)); } // Number 
    //if(Sign == 3) { ReturnStr[a] = char(random(0,5)); } // SpecialChar 
    }
  return ReturnStr;
}

void SetBackLightBrightness ( byte Brightness) // Brightness in Percent 0 - 100
{
  wantedLEDBrightness = map(Brightness, 0, 100, 0, 255);
  ledcWrite(LcdBacklightCh, wantedLEDBrightness);
  if (debug) 
    {
    Serial.print ("Bk Brght chng from: ");
    Serial.print (currentLEDBrightness);
    Serial.print (" to: ");
    Serial.println (wantedLEDBrightness);
    }
  currentLEDBrightness = wantedLEDBrightness;  
}

void mydelay (unsigned long Tmydelayinms) 
{
  previousMillis = millis();
  do 
    { 
    currentMillis = millis();
#ifdef VOICEASSISTANT
    espalexa.loop();
#endif
    yield();
    //esp_task_wdt_reset();
    } while (!(millis() - previousMillis >= Tmydelayinms)); 
}

void BackLightBrightnessWorker() // Manages Fadind from Backlight Brighness 
{
if (!(wantedLEDBrightness == currentLEDBrightness))
  {  
  if (wantedLEDBrightness < currentLEDBrightness)
    {
     currentLEDBrightness--;  
     ledcWrite(LcdBacklightCh, currentLEDBrightness); 
     mydelay(5);
    } else
    {
     currentLEDBrightness++;   
     ledcWrite(LcdBacklightCh, currentLEDBrightness); 
     mydelay(5); 
    }
  }
}

void FadeBackLightBrightness ( byte Brightness) // Background Light Brightness in Percent 0 - 100
{
  wantedLEDBrightness = map(Brightness, 0, 100, 0, 255);
  if (debug) 
    {
    Serial.print ("Bk Brght chng from: ");
    Serial.print (currentLEDBrightness);
    Serial.print (" to: ");
    Serial.println  (wantedLEDBrightness);
    }
}

String GenerateSSID(bool newSSID)
{
String tmpStr = "GaesteLAN000";
if (newSSID)
  {
  for (int a = 9;a < 12;a++)
    {
    tmpStr[a] = char(random(48,57));  // Number 
    }
  OLDGenSSID = tmpStr; 
  } else
  {
  if (OLDGenSSID.length() > 10) { tmpStr = OLDGenSSID; } else
    {
    for (int a = 9;a < 12;a++)
      {
      tmpStr[a] = char(random(48,57));  // Number 
      } 
    OLDGenSSID = tmpStr;
    } 
  }
  return tmpStr;
}

#ifdef VOICEASSISTANT
void StatusGuestWLAN(EspalexaDevice* d) 
{
  //int ActualValue = (d->getValue());
  byte Percent = (d->getPercent());
  if (Tr64ConfigAvail)
    {
    if (Percent < 5)
      {
      WLANState  = false;
      WLANStateHelp = true;
      if (debug) { Serial.println ( "Alexa Guest WLAN TURN OFF Command received." ); }
      }  else
      {
      WLANState  = true;
      WLANStateHelp = true;
      if (debug) { Serial.println ( "Alexa Guest WLANTURN ON Command received." ); }
      }
    }  else { d->setValue(700); }  
}
#endif

#ifdef VOICEASSISTANT
void BackLightBrightness(EspalexaDevice* d) 
{
  byte Percent = (d->getPercent());
  if (WLANState)
    {  
    //SetBackLightBrightness(Percent);
    FadeBackLightBrightness(Percent);
    }
}
#endif

void SetRTCClock()
  {
  rtc.setTime(ntpClient.getUnixTime()); //Sommer Zeit  Standard in die RTC übertragem  
  byte AMonth = rtc.getMonth(); 
  if ( ((AMonth  > 10 ) || (AMonth < 2 )) && ((!(AMonth == 2)) || (!(AMonth == 9))) )
    { 
    if (debug) { Serial.println ( "Zeitkorrektur WinterZeit" ); }
    rtc.setTime(ntpClient.getUnixTime()-3600); //Winterzeitumstellung
    }
  if (AMonth == 2) // Monat März, Winter auf Sommerzeitumstellung am letzten Sonntag des Monats 31 Tage.
    {
    byte ADayofMonth = rtc.getDay();
    byte ADayofWeek = rtc.getDayofWeek(); 
    byte CalcDaysleft = 31 - ADayofMonth; // ADayofMonth
    byte SundaysLeft = CalcDaysleft / 7;
    if (!(((SundaysLeft == 0) && (ADayofWeek == 7))))
      {
       if (debug) { Serial.println ( "Zeitkorrektur WinterZeit - Monat März" ); }
       rtc.setTime(ntpClient.getUnixTime()-3600); //Winterzeitumstellung 
      }
    }
  if (AMonth == 9) // Monat Oktober,  Sommer aufWinter zeitumstellung am letzten Sonntag des Monats 31 Tage.
    {
    byte ADayofMonth = rtc.getDay();
    byte ADayofWeek = rtc.getDayofWeek(); 
    byte CalcDaysleft = 31 - ADayofMonth; // ADayofMonth
    byte SundaysLeft = CalcDaysleft / 7;
    if ( (SundaysLeft == 0) && (ADayofWeek == 7) )
      {
       if (debug) { Serial.println ( "Zeitkorrektur WinterZeit - Monat Oktober" ); }
       rtc.setTime(ntpClient.getUnixTime()-3600); //Winterzeitumstellung 
      }
    } 
  if (debug) { Serial.println (rtc.getDate(true)); }
  if (debug) { Serial.println (rtc.getTime()); }
  }

void listAllFiles()
{
Serial.println("Available Files on SPIFFS:");  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
      Serial.print("FILE: ");
      Serial.println(file.name());
      file = root.openNextFile();
  }
  file.close(); 
}

void GenerateQRCodefromText (char* Message,byte XStartPoint,byte YStartPoint,byte qrcodecapacity, byte qrcodeErrorTolerance, bool showinfo ) // maximal 132 Zeichen QR Code !!!
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
  if (showinfo)
    {
    u8g2.drawFrame(XStartPoint,YStartPoint +qrcode.size+3, qrcode.size+4,15) ;   
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(XStartPoint+ 3, YStartPoint + qrcode.size+13,"QRCode");  
    }
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void QRCodeURL (String URL)
{
  String ClTextQRCode = URL;
  int StrLngth = ClTextQRCode.length()+1;
  char ChrBuff[StrLngth];
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = 0; } 
  for (int a = 0; a < StrLngth;a++){ ChrBuff[a] = ClTextQRCode[a]; }
  GenerateQRCodefromText(ChrBuff,85,2,5, 2,true); // URL in QR Code Form 
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
  GenerateQRCodefromText(ChrBuff,85,2,5, 2,true); // WLAN Credentials in QR Code Form 
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
 #define MaxDisplayChars 29
 #define MaxDisplayLines 4
 #define TextY_Pixel_Offset 21
 #define TextY_Pixel_SpaceBLines 10
 #define TextX_Pixel_Offset 2
 int ErrorTXTLength = Error.length();
 u8g2.clearBuffer();          // clear the internal memory
 u8g2.drawFrame(0,1,40,12);
 u8g2.drawFrame(0,12,125,50);
 u8g2.setFont(u8g2_font_squeezed_b7_tr);
 u8g2.drawStr(3,10, "Fehler!");
 if (ErrorTXTLength < 28) { u8g2.setFont(u8g2_font_squeezed_b7_tr); } else { u8g2.setFont(u8g2_font_4x6_tf); } 
 if (ErrorTXTLength < MaxDisplayChars + 1) // Zeilenlänge ist bei Font u8g2_font_4x6_tf maximal 32 
  {  
  u8g2.drawStr(TextX_Pixel_Offset,TextY_Pixel_Offset,Error.c_str());  
  } else // Split into more Lines
  {      
  int NeedLines = ErrorTXTLength / MaxDisplayChars;
  int NeedLines_Offset =  ErrorTXTLength % MaxDisplayChars;
  int TextY_Pixel_Offset_Calc = 0;
  int TextX_Pixel_Offset_Calc = 0;
  char Messagetext[MaxDisplayChars +3];
  int ccm = 0;
  int tmp = 0; 
  if (NeedLines_Offset > 0) { NeedLines++;}
  if (NeedLines > MaxDisplayLines) { NeedLines = MaxDisplayLines; } // Truncate if lines not fit in Display
  for (tmp = 0;tmp <= NeedLines -1 ;tmp++)
    {
    for (ccm = 0; ccm <= MaxDisplayChars+2; ccm++) {Messagetext[ccm] = 0; } 
    if (tmp == NeedLines -1) 
      { 
      for (ccm = 0; ccm <= NeedLines_Offset; ccm++) {Messagetext[ccm] = Error[ccm+tmp+(tmp*MaxDisplayChars)]; }   
      Messagetext[MaxDisplayChars+1] = 0;  
      } else
      {    
      for (ccm = 0; ccm <= MaxDisplayChars; ccm++) {Messagetext[ccm] = Error[ccm+tmp+(tmp*MaxDisplayChars)]; }   
      Messagetext[MaxDisplayChars+1] = 0;
      }
    TextY_Pixel_Offset_Calc = tmp * TextY_Pixel_SpaceBLines + TextY_Pixel_Offset;
    TextX_Pixel_Offset_Calc = TextX_Pixel_Offset;
    u8g2.drawStr(TextX_Pixel_Offset_Calc,TextY_Pixel_Offset_Calc,Messagetext);   
    }  
  }
  u8g2.sendBuffer();          // transfer internal memory to the display
  esp_task_wdt_reset();
  delay(6000);
  esp_task_wdt_reset();
}

bool ConfigtoSave(SysConfig &MyConfig) {
  File file = SPIFFS.open("/SysGlobal-config", "w");
  if (file) {
    file.write(reinterpret_cast<byte*>(&MyConfig), sizeof(MyConfig));   // Serialisierung
    file.close();
    return true;
  }
  return false;
}

// Funktion zum einlesen der Daten aus der Datei
bool ConfigtoRead(SysConfig &MyConfig) {
  File file = SPIFFS.open("/SysGlobal-config", "r");
  if (file) {
    file.read(reinterpret_cast<byte*>(&MyConfig), sizeof(MyConfig));  // Deserialisierung
    file.close();
    return true;
  }
  return false;
}
