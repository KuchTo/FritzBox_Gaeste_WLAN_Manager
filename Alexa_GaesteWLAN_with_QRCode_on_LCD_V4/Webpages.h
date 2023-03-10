
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<style> 
{box-sizing: border-box; }
body {font-family: Arial, Helvetica, sans-serif;}
table, th, td {  border:1px solid black;}
header {background-color: #666;padding: 20px;text-align: center;font-size: 25px;color: white;;width: 100%;}
nav {float: left;width:22%;height: 300px;background: #ccc;padding: 20px;}
nav ul {list-style-type: none;padding: 0;}
article {float: center;padding: 20px;height: 300px;width: 100%;background-color: #f1f1f1;text-align:center;}
section::after {content: '';display: table;clear: both;}
footer {background-color: #177;padding: 20px;text-align: center;color: whitesmoke;;width: 100%;}
@media (max-width: 700px) {nav, article {width:100%;height:100%;}
}
</style>
<head>
<title>WLAN Schalter</title>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
</head>
<body>
<!-- Dedicated to Bianca, a valuable , beloved Person -->
<header>
<h3>Gäste WLAN Manager</h3>
</header>
<section>
<nav>
<strong>Navigation</strong>
<ul>
<li><a href='/StateInfo' target='MainIFrame' >Status des Systems</a></li>
<li><a href='/SetCntdwnTime' target='MainIFrame' >Zeit Konfiguration</a></li>
<li><a href='/tr64' target='MainIFrame' >TR64 Konfiguration</a></li>
<li><a href='/update' target='MainIFrame' >Firmware Update (!)</a></li>
</ul>
<strong>Bedienung</strong>
<form action='/capi' target='MainIFrame'>
<p></p>
<input type='submit' class='btn btn-primary' name='WLAN_Sw_T' value='WLAN Ein/Aus'>
<p></p>
<input type='submit' class='btn btn-primary' name='Reboot_Sys' value='System Reboot'>
</nav>
<article style=text-align:left><iframe src='/StateInfo' width='350px' height='299px' style='border:none;'  title='Informations' name = 'MainIFrame'></iframe>
</article>
</section>
<footer><p>Programmiert von Tobias Kuch V1.0 -09.02.2023-</p></footer>
</body>
</html>
)rawliteral";

const char StateInfo_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<style> 
{box-sizing: border-box; }
body {font-family: Arial, Helvetica, sans-serif;}
table, th, td {  border:1px solid black;}
</style>
<head>
<title>HTML Redirect</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv='refresh' content='60; URL=/StateInfo'/> 
</head>
<body>
<p>Gästenetzwerk Informationen:</p>
<table style='width:300px'><tr><td>Status</td><td>SSID</td><td>Passwort</td></tr><tr>
%GWLANPARAMETER%
</tr></table><p></p>
<p>Verbleibende Zeit:</p>
<table style='width:300px'><tr><td>Tage</td><td>Stunden</td><td>Minuten</td></tr><tr>
%TIMEINFORMATIONS%
</tr>
</table>
</body>
</html>
)rawliteral";

const char SetTimeToWLANDisable_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<body>
<p>Zeitraum des aktiven Gästenetzwerks:</p>
<form action='/capi'>
<table><tr><td>
<div class='form-group'>
<select class='form-control' name='Days'>
%OPTION1%
</select></div></td><td>
<div class='form-group'>
<select class='form-control' name='Hours'>
%OPTION2%
</select></div></td><td>
<div class='form-group'>
<select class='form-control' name='Minutes'>
%OPTION3%
</select></div></td></tr></table> 
<p></p>
<div class='form-group'>
<input type='submit' class='btn btn-primary' name='TimeSelect' value='Speichern'>
</div></form></body></html>
)rawliteral";


const char tr64_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<body>
<form action="/capi">
<table>
  <tr>
  <td>Benutzer</td>
  <td><div class='form-group'><input type='text' class='form-control' name='TR64_User' placeholder='Fritzbox Benutzer' required='required'></div></td>
  </tr>
  
  <tr>  
  <td>Passwort</td>
  <td><div class='form-group'><input type='text' class='form-control' name='TR64_Password' placeholder='Fritzbox Passwort' required='required'></div></td>
  </tr>
  
  <tr>
  <td>IP Adresse</td>
  <td><div class='form-group'><input type='text' class='form-control' name='TR64_IP' placeholder='IP Adresse Ihrer FritzBox' required='required'></div></td>
  </tr>

  <tr>
  <td>TR 64 Port</td>
  <td><div class='form-group'><input type='text' class='form-control' name='TR64_PORT' placeholder='49000 (Standardport)' required='required'></div></td>
  </tr>
  
</table> 
<p></p><div class='form-group'><input type='submit' class='btn btn-primary' name='Store_TR64_Value' value='TR64 Konfiguration speichern'></div> 
</form>
<small>Nach Absenden ist ein Neustart des Systems notwendig.</small>
</body>
</html> 
)rawliteral";


String DynamicHTMLContent(const String& var){


if(var == "PERCENT"){
    String Percent = "%";
    return Percent  ;
  }
   
if(var == "GWLANPARAMETER"){
    String GWlanStatus = "";
    String StrLocalIPAddr = WiFi.localIP().toString().c_str();
    String State = "";
    if (WLANState) { State = "Aktiviert"; } else { State = "Deaktiviert"; }     
    GWlanStatus  = "<td>"+ State + "</td><td>" + GSSID + "</td><td>" + GWANPW + "</td>";   
    return GWlanStatus  ;
  }
if(var == "TIMEINFORMATIONS"){
    String GWlanStatus = "";
    GWlanStatus  = "<td>"+ String(CntDwnDays) + "</td><td>" + String(CntDwnHours) + "</td><td>" + String(CntDwnMinutes) + "</td>";   
    return GWlanStatus  ;
  }
if(var == "OPTION1"){
    String GWlanStatus = "";
    String OptionActive =" selected";
    String Str0 = "";
    String Str1 = "";
    String Str2 = ""; 
    String Str3 = ""; 
    String Str4 = ""; 
    String Str5 = ""; 
    String Str7 = ""; 
    String Str9 = "";
    if (MyConfig.Days == 0) { Str0 = OptionActive; }
    if (MyConfig.Days == 1) { Str1 = OptionActive; }
    if (MyConfig.Days == 2) { Str2 = OptionActive; }
    if (MyConfig.Days == 3) { Str3 = OptionActive; }
    if (MyConfig.Days == 4) { Str4 = OptionActive; }
    if (MyConfig.Days == 5) { Str5 = OptionActive; }
    if (MyConfig.Days == 7) { Str7 = OptionActive; }
    if (MyConfig.Days == 9) { Str9 = OptionActive; }
    GWlanStatus = "<option value='0'"+ Str0 +">0 Tage</option><option value='1'"+ Str1 +">1 Tag</option><option value='2'"+ Str2 +">2 Tage</option><option value='3'"+ Str3 +">3 Tage</option><option value='4'"+ Str4 +">4 Tage</option><option value='5'"+ Str5 +">5 Tage</option><option value='7'"+ Str7 +">7 Tage</option><option value='9'"+ Str9 +">9 Tage</option>";  
    return GWlanStatus  ;
  } 

if(var == "OPTION2"){
    String GWlanStatus = "";
    String OptionActive =" selected";
    String Str0 = "";
    String Str1 = "";
    String Str2 = ""; 
    String Str4 = ""; 
    String Str6 = ""; 
    String Str8 = ""; 
    String Str10 = "";
    String Str12 = "";
    String Str15 = "";
    String Str20 = ""; 
    String Str22 = ""; 
    if (MyConfig.Hours == 0) { Str0 = OptionActive; }
    if (MyConfig.Hours == 1) { Str1 = OptionActive; }
    if (MyConfig.Hours == 2) { Str2 = OptionActive; }
    if (MyConfig.Hours == 4) { Str4 = OptionActive; }
    if (MyConfig.Hours == 6) { Str6 = OptionActive; }
    if (MyConfig.Hours == 8) { Str8 = OptionActive; }
    if (MyConfig.Hours == 10) { Str10 = OptionActive; }
    if (MyConfig.Hours == 12) { Str12 = OptionActive; }
    if (MyConfig.Hours == 15) { Str15 = OptionActive; }
    if (MyConfig.Hours == 20) { Str20 = OptionActive; }
    if (MyConfig.Hours == 22) { Str22 = OptionActive; }   
    GWlanStatus = "<option value='0'"+ Str0 +">0 Stunden</option><option value='1'"+ Str1 +">1 Stunde</option><option value='2'"+ Str2 +">2 Stunden</option><option value='4'"+ Str4 +">4 Stunden</option><option value='6'"+ Str6 +">6 Stunden</option><option value='8'"+ Str8 +">8 Stunden</option><option value='10'"+ Str10 +">10 Stunden</option><option value='12'"+ Str12 +">12 Stunden</option><option value='15'"+ Str15 +">15 Stunden</option><option value='20'"+ Str20+">20 Stunden</option><option value='22'"+ Str22+">22 Stunden</option>";
    return GWlanStatus  ;
  } 
 
if(var == "OPTION3"){
    String GWlanStatus = "";
    String OptionActive =" selected";
    String Str0 = "";
    String Str1 = "";
    String Str2 = ""; 
    String Str3 = ""; 
    String Str4 = ""; 
    String Str5 = ""; 
    if (MyConfig.Minutes == 0) { Str0 = OptionActive; }
    if (MyConfig.Minutes == 10) { Str1 = OptionActive; }
    if (MyConfig.Minutes == 20) { Str2 = OptionActive; }
    if (MyConfig.Minutes == 30) { Str3 = OptionActive; }
    if (MyConfig.Minutes == 40) { Str4 = OptionActive; }
    if (MyConfig.Minutes == 50) { Str5 = OptionActive; }
    GWlanStatus = "<option value='0'"+ Str0 +">0 Minuten</option><option value='10'"+ Str1 +">10 Minuten</option><option value='20'"+ Str2 +">20 Minuten</option><option value='30'"+ Str3 +">30 Minuten</option><option value='40'"+ Str4 +">40 Minuten</option><option value='50'"+ Str5 +">50 Minuten</option>";   
    return GWlanStatus  ;
  } 

  return String();
}


const char redirect_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><title>HTML Redirect</title><meta name="viewport" content="width=device-width, initial-scale=1">   <meta http-equiv='refresh' content='3; URL=/StateInfo'/> 
</head><body><h3>Daten empfangen</h3><small>Verarbeite ..</small></body></html>
)rawliteral";

const char InputError_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><title>HTML Redirect</title><meta name="viewport" content="width=device-width, initial-scale=1"><meta http-equiv='refresh' content='3; URL=/StateInfo'/> 
</head><body><small>Eingabe ungültig !</small></body></html>
)rawliteral";

void SetWebPages()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", index_html);   
    });

  server.on("/tr64", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", tr64_html);   
    });
  
  server.on("/StateInfo", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", StateInfo_html,DynamicHTMLContent);   
    });
  
  server.on("/SetCntdwnTime", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", SetTimeToWLANDisable_html,DynamicHTMLContent);   
    });
  
  server.on("/capi", HTTP_GET, [](AsyncWebServerRequest *request){
      int paramsNr = request->params();
      // Serial.println(paramsNr);
      String ParamName[paramsNr];
      String ParamValue[paramsNr];
      String StoreStr = "";
      byte a = 0;
      bool rebootrequired = false;
      for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        ParamName[i] = p->name();
        ParamValue[i] = p->value();        
        if (debug) 
          { 
          Serial.print("Param name:");
          Serial.print( ParamName[i]);
          Serial.print("  value:");
          Serial.println(ParamValue[i]);
          }
        }

      if ((paramsNr == 1) && (ParamName[0] == "Reboot_Sys")) 
        {
        request->send(404, "text/html", "Reboot gestartet.");
        delay(2000);
        rebootrequired = true;
        }
             
      if ((paramsNr == 1) && (ParamName[0] == "WLAN_Sw_T")) 
        {
        if(WLANState)
          {
          CntDwnSeconds = 1; 
          CntDwnMinutes = 0;
          CntDwnHours = 0;
          CntDwnDays = 0;   
          WLANState  = false;
          WLANStateHelp = true;
          }  else
          {
          WLANState  = true;
          WLANStateHelp = true;
          }
        } 
     
     if ((paramsNr == 4) && (ParamName[3] == "TimeSelect")) // New TimeSelect Configuration Settings
        {
        int Day_tmp = ParamValue[0].toInt();
        int Hours_tmp = ParamValue[1].toInt();
        int Minutes_tmp = ParamValue[2].toInt();
        if ( (Day_tmp == 0) & (Hours_tmp == 0) & (Minutes_tmp == 0) )
          {
           request->send_P(200, "text/html",InputError_html); 
          } else // Zeitwert gülig
          {      
          MyConfig.Days = Day_tmp;
          MyConfig.Hours = Hours_tmp;
          MyConfig.Minutes = Minutes_tmp;
          bool result = ConfigtoSave(MyConfig);
          CntDwnSeconds = MyConfig.Seconds; // Associate
          CntDwnMinutes = MyConfig.Minutes;
          CntDwnHours = MyConfig.Hours;
          CntDwnDays = MyConfig.Days; 
          }
        }
              
      if ((paramsNr == 5) && (ParamName[4] == "Store_TR64_Value")) // NewTR64 Configuration Settings
        {
        // Serial.println("Storing TR64 Credentials..");
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
       
      request->send_P(200, "text/html", redirect_html);
      if (rebootrequired) {ESP.restart(); }
    });
  server.onNotFound([](AsyncWebServerRequest *request){
#ifdef VOICEASSISTANT     
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
        {
        //whatever you want to do with 404s
        request->send(404, "text/plain", "404 - Page Not found");
        }
#else
        request->send(404, "text/plain", "404 - Page Not found");
#endif      
      });
}
