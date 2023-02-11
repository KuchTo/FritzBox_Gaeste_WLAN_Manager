//callback functions
void C0Changed(EspalexaDevice* dev);

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


void mydelay (unsigned long Tmydelayinms) 
{
  previousMillis = millis();
  do 
    { 
    currentMillis = millis();
    espalexa.loop();
    yield();
   // esp_task_wdt_reset();
    } while (!(millis() - previousMillis >= Tmydelayinms)); 
}

void StatusGuestWLAN(EspalexaDevice* d) 
{
  //int ActualValue = (d->getValue());
  byte Percent = (d->getPercent());
  if (Tr64ConfigAvail)
    {
    if (Percent < 10)
      {
      WLANState  = false;
      WLANStateHelp = true;
      if (debug) { Serial.println ( "Alexa TURN OFF Command received." ); }
      }  else
      {
      WLANState  = true;
      WLANStateHelp = true;
      if (debug) { Serial.println ( "Alexa TURN ON Command received." ); }
      }
    }  else { d->setValue(700); }  
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
