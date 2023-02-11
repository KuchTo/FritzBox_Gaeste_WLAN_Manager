

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <title>Tobis Luxus Gaeste WLAN Schalter - mit Alexa Unterstuetzung</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>
<h2>Startseite</h2>
</body>

</html> 
  
)rawliteral";


const char tr64_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Tobis Luxus Gaeste WLAN Schalter - mit Alexa Unterstuetzung</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>

<body>
<h2>TR 64 Protokollkonfiguration</h2>

<form action="/capi">
  <div class='form-group'>
    <label>TR 64 Benutzer *</label> <input type='text' class='form-control' name='TR64_User' placeholder='Fritzbox Benutzername eingeben' required='required'>
  </div>
  <div class='form-group'>
    <label>TR 64 Passwort *</label> <input type='text' class='form-control' name='TR64_Password' placeholder='Passwort des Benutzers' required='required'>
  </div>
  <div class='form-group'>
    <label>IP Adresse *</label> <input type='text' class='form-control' name='TR64_IP' placeholder='IP Adresse Ihrer FritzBox' required='required'>
  </div>
  <div class='form-group'>
    <label>TR 64 Port *</label> <input type='text' class='form-control' name='TR64_PORT' placeholder='49000 (Standardport)' required='required'>
  </div>
  <div class='form-group'>
    <input type='submit' class='btn btn-primary' name='Store_TR64_Value' value='Protokollkonfiguration speichern'>
  </div><small>Felder markiert mit * sind Pflichtfelder.</small>
  </div><small>Nach Absenden der TR 64 Protokolldaten ist ein Nustart des Systems notwendig.</small>
</form>

</body>
</html> 
  
)rawliteral";


String getLocalIPRedir(const String& var){
  //Serial.println(var);
  if(var == "IPPLACEHOLDERA"){
    String HTMLIPAdress = "";
    StrLocalIPAddr = WiFi.localIP().toString().c_str();
    HTMLIPAdress = "<meta http-equiv='refresh' content='5; URL=http://" + StrLocalIPAddr + "'/> ";
    return HTMLIPAdress ;
  }
  if(var == "IPPLACEHOLDERB"){
    String HTMLIPAdress = "";
    StrLocalIPAddr = WiFi.localIP().toString().c_str();
    HTMLIPAdress = "<p>If you are not redirected in five seconds, <a href='http://" + StrLocalIPAddr + "'>click here</a>.</p>";
    return HTMLIPAdress ;
  }
  
  return String();
}


const char redirect_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
   <title>HTML Redirect</title>
   <meta name="viewport" content="width=device-width, initial-scale=1">
   %IPPLACEHOLDERA%
</head>
<body>
   <h2>Formulardaten empfangen.</h2>
   <p>Weiterleitung an Startseite in 5 Sekunden.</p>
   %IPPLACEHOLDERB%  
</body>
</html>
)rawliteral";


void SetWebPages()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", index_html);   
    });

  server.on("/tr64", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", tr64_html);   
    });
    
  server.on("/capi", HTTP_GET, [](AsyncWebServerRequest *request){
      int paramsNr = request->params();
      Serial.println(paramsNr);
      String ParamName[paramsNr];
      String ParamValue[paramsNr];
      String StoreStr = "";
      byte a = 0;
      bool rebootrequired = false;
      for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        ParamName[i] = p->name();
        ParamValue[i] = p->value();        
        Serial.print("Param name:");
        Serial.print( ParamName[i]);
        Serial.print("  value:");
        Serial.println(ParamValue[i]);
        }
       if (ParamName[4] == "Store_TR64_Value") // NewTR64 Configuration Settings
        {
        //Serial.println("Storing TR64 Credentials..");
        File file = SPIFFS.open("/tr64-config", FILE_WRITE);
        for (a = 0; a < paramsNr-1;a++)
          {
          StoreStr = "";
          StoreStr = ParamValue[a];
          file.println(StoreStr);
          }       
        file.close();
        rebootrequired = true;
        }        
      request->send_P(200, "text/html", redirect_html,getLocalIPRedir);
      if (rebootrequired) {ESP.restart(); }
    });
  server.onNotFound([](AsyncWebServerRequest *request){
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
        {
        //whatever you want to do with 404s
        request->send(404, "text/plain", "404 Not found");
        }
      });
}
