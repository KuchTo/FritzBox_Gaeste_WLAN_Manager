#include <tr064.h>

const char FRITZBOX_IP[] = "111.111.111.111";
const int FRITZBOX_PORT = 4900;
const char USER[] = "MyTr64User";
const char PASSWORD[] = "MyTr64User"; // <-- ändern

TR064 tr064_connection(FRITZBOX_PORT, FRITZBOX_IP, USER, PASSWORD);



bool SwitchGuestWifi(bool status) 
{
  String params[][2] = {{"NewEnable", "0"}};
  String req[][2] = {{}};
  if (status) 
    {
    params[0][1] = "1";
    Serial.println ("Set WLAN status: on");
    } else {
    Serial.println ("Set WLAN status: off");
    }
  if (tr064_connection.action("urn:dslforum-org:service:WLANConfiguration:3", "SetEnable", params, 1, req, 2))
    {
    return true;
    } else
    {
    return false;
    } 
}


bool NewSSIDGuestWifi(String NewSSID) 
{
  String params[][2] = {{"NewSSID",  NewSSID}};
  String req[][2] = {{}};
 
  if (tr064_connection.action("urn:dslforum-org:service:WLANConfiguration:3", "SetSSID", params, 2, req, 2))
    { 
    Serial.println ("SetNewSSID true");
    return true;
    } else
    {
    Serial.println ("SetNewSSID false");
    return false;
    }
}


bool SetPWGuestWifi(String StrPassword) 
{   
  //String params[6][2] = {{ "NewWEPKey0", "" }, { "NewWEPKey1", "" }, { "NewWEPKey2", ""  }, { "NewWEPKey3", ""  }, { "NewPreSharedKey", "48F93E91801FB6E2EE1374E5B9026CEA92468FC0C9CBD7DE2B3D139DD04B9850" }, { "NewKeyPassphrase", StrPassword }};
  String params[6][2] = {{ "NewWEPKey0", "" }, { "NewWEPKey1", "" }, { "NewWEPKey2", ""  }, { "NewWEPKey3", ""  }, { "NewPreSharedKey", "A487C40982581DB9F71090303754A53E9015409AA2489697F83278765257F01C" }, { "NewKeyPassphrase", StrPassword }};
  String req[6][2] = {{}};

  if (tr064_connection.action("urn:dslforum-org:service:WLANConfiguration:3", "SetSecurityKeys", params, 6, req, 2)) 
    { 
    Serial.println ("SetPWGuestWifi true");
    return true;
    } else
    {
    Serial.println ("SetPWGuestWifi false");
    return false;
    }
}



String GenerateWLANPassword()
{
 String tmpStr = "123456789";
 //char BigChar = random(65,90);
 //char littleChar = random(97,122);
 //char Num = random(48,57);
 // char SpecialChar = random(0,5);

for (int a = 0;a < 9;a++)
{
 byte Sign = random(0,3);
 if(Sign == 0) { tmpStr[a] = char(random(65,90)); } // BigChar
 if(Sign == 1) { tmpStr[a] = char(random(97,122)); } // littleChar 
 if(Sign == 2) { tmpStr[a] = char(random(48,57)); } // Number 
 if(Sign == 3 ) { tmpStr[a] = char(random(0,5)); } // SpecialChar 
}
 
 return tmpStr;
}


String GenerateSSID()
{
 String tmpStr = "GaesteLAN000";
 for (int a = 9;a < 12;a++)
  {
  tmpStr[a] = char(random(48,57));  // Number 
  }
 return tmpStr;
}
