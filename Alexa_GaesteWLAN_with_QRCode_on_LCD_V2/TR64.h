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

void Dialing() {
 
  String params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**799"}};
  String req[][2] = {{}};
  
  tr064_connection.action("X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0);
  //connection.action("urn:dslforum-org:service:X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0); 
  // without loading available services through init() you have to set the url
  //connection.action("X_VoIP:1", "X_AVM-DE_DialNumber", params, 1, req, 0, "/upnp/control/x_voip");
}


#endif
