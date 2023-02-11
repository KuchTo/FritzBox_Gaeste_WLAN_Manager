//callback functions
void C0Changed(EspalexaDevice* dev);

Espalexa espalexa;
AsyncWebServer server(80);
WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", ((1*60*60))); // IST = GMT + 1:00



void mydelay (unsigned long Tmydelayinms) 
{
  previousMillis = millis();
  do 
    { 
    currentMillis = millis();
    espalexa.loop();
   // esp_task_wdt_reset();
    } while (!(millis() - previousMillis >= Tmydelayinms)); 
}



void StatusGuestWLAN(EspalexaDevice* d) 
{
 
   int ActualValue = (d->getValue());
   byte Percent = (d->getPercent());

  if (Percent == 0)
  {
      WLANState  = false;
      WLANStateHelp = true;
      Serial.println ( "Alexa TURN OFF Command received." );
     //digitalWrite(LCD_BACKLIGHT_PIN, LOW);
  }  else
  {
    WLANState  = true;
    WLANStateHelp = true;
    //digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
    Serial.println ( "Alexa TURN ON Command received." );
  }
  //  d->setValue(700);  
}
