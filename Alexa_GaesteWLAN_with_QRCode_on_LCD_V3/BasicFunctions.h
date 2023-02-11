//callback functions
//void C0Changed(EspalexaDevice* dev);

Espalexa espalexa;
AsyncWebServer server(80);
WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((1*60*60))); // IST = GMT + 1:00


void IRAM_ATTR onTimer()      
  {
 // portENTER_CRITICAL_ISR(&timerMux);
  //Calculate Time 24 Hour Format
  SecInterruptOccured = true;
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
    }     
 // portEXIT_CRITICAL_ISR(&timerMux);
}
//Interrupts ende

String GenerateWLANPassword(byte PwLength)
{
  if (PwLength < 8)  { PwLength = 8; }
  if (PwLength > 12)  { PwLength = 12; }
  char tmpStr[PwLength+3];
  String ReturnStr;
  for (int a = 0;a < PwLength;a++)
    {
    byte Sign = random(0,3);
    if(Sign == 0) { tmpStr[a] = char(random(65,90)); } // BigChar
    if(Sign == 1) { tmpStr[a] = char(random(97,122)); } // littleChar 
    if(Sign == 2) { tmpStr[a] = char(random(48,57)); } // Number 
    if(Sign == 3) { tmpStr[a] = char(random(0,5)); } // SpecialChar 
    }
  tmpStr[PwLength+1] = char(10);
  tmpStr[PwLength+2] = char(13);  
  tmpStr[PwLength+3] = char(0);
  ReturnStr = tmpStr;
  return ReturnStr;
}

void SetBackLightBrightness ( byte Brightness) // Brightness in Percent 0 - 100
{
  wantedLEDBrightness = map(Brightness, 0, 100, 0, 255);
  ledcWrite(LcdBacklightCh, wantedLEDBrightness);
  if (debug) 
    {
    Serial.print ("LED Brightness changes from: ");
    Serial.print (currentLEDBrightness);
    Serial.print (" to: ");
    Serial.println  (wantedLEDBrightness);
    }
  currentLEDBrightness = wantedLEDBrightness;  
}

void mydelay (unsigned long Tmydelayinms) 
{
  previousMillis = millis();
  do 
    { 
    currentMillis = millis();
    espalexa.loop();
    yield();
    esp_task_wdt_reset();
    } while (!(millis() - previousMillis >= Tmydelayinms)); 
}

void BackLightBrightnessWorker() // Manages Fadind from Backlight Brighness 
{
if (!(wantedLEDBrightness == currentLEDBrightness))
  {  
  if (wantedLEDBrightness < currentLEDBrightness)
    {
     //Serial.print ("Hintergrundbeleuchtungsänderung nach unten auf:");
     //Serial.println (currentLEDBrightness);
     // Serial.println (wantedLEDBrightness);
     currentLEDBrightness--;  
     ledcWrite(LcdBacklightCh, currentLEDBrightness); 
     mydelay(5);
    } else
    {
     //Serial.print ("Hintergrundbeleuchtungsänderung nach oben auf: ");
     //Serial.println (wantedLEDBrightness);
     //Serial.println (currentLEDBrightness);  
     currentLEDBrightness++;   
     ledcWrite(LcdBacklightCh, currentLEDBrightness); 
     mydelay(5); 
    }
  }
}

void FadeBackLightBrightness ( byte Brightness) // Background Light Brightness in Percent 0 - 100
{
  wantedLEDBrightness = map(Brightness, 0, 100, 0, 255);
  Serial.print ("LED Brightness changes from: ");
  Serial.print (currentLEDBrightness);
  Serial.print (" to: ");
  Serial.println  (wantedLEDBrightness);
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

void BackLightBrightness(EspalexaDevice* d) 
{
  byte Percent = (d->getPercent());
  if (WLANState)
    {  
    //SetBackLightBrightness(Percent);
    FadeBackLightBrightness(Percent);
    }
}


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
