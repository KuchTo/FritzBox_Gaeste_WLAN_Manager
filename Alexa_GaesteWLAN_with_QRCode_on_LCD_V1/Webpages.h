
void SetWebPages()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Stay tuned.. In the Future , here comes to cool stuff :) .. Tobias");
    });
  server.on("/capi", HTTP_GET, [](AsyncWebServerRequest *request){
      int paramsNr = request->params();
      Serial.println(paramsNr);
      for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
        }
      request->send(200, "text/plain", "message received");
    });
  server.onNotFound([](AsyncWebServerRequest *request){
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
        {
        //whatever you want to do with 404s
        request->send(404, "text/plain", "404 Not found");
        }
      });
}
