#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

DNSServer dnsServer;
AsyncWebServer server(80);
String Response;

void responseGenerator() {
  wifi_mode_t returnMode = WiFi.getMode();

  WiFi.mode(WIFI_STA);

  String options = "";
  for (int i = 0, n = WiFi.scanNetworks(); i < n; ++i) {
    if (WiFi.RSSI(i) < 80) {
      options += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    }
  }

  WiFi.mode(returnMode);

  String select = "<select name=\"ssid\">" + options + "</select>";
  String pass = "<input type=\"text\" autocomplete=\"off\" placeholder=\"password\"  name=\"pass\">";
  String form = "<form><label>Choose an AP:</label><br>" + select + "<br><br>" + "<label>Type Password:</label><br>" + pass + "<br><br>" + "<input type=\"submit\" value=\"Submit\"></form>";

  String body = form;

  String response = "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
                    + body
                    + "</body></html>";

  ::Response = response;
}

bool connectToAP(String ssid, String pass) {
  wifi_mode_t returnMode = WiFi.getMode();
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pass.c_str());
  delay(1000);
  if (WiFi.status() != WL_CONNECTED) delay(1000);
  if(WiFi.status() == WL_CONNECTED) return true;
  else{
    responseGenerator();
    WiFi.mode(returnMode);
    return false;
  }
}

class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
      //request->addInterestingHeader("ANY");
      return true;
    }

    void handleRequest(AsyncWebServerRequest *request) {
      int paramsNr = request->params();

      if (paramsNr == 2) {
        AsyncWebParameter* p1 = request->getParam(0);
        AsyncWebParameter* p2 = request->getParam(1);
        if (p1->name() == "ssid" && p2->name() == "pass") {
          if(connectToAP(p1->value(), p2->value())){
            dnsServer.stop();
            // ... Actions after 
            return;
          }else return;
        }
      }

      request->send(200, "text/plain", ::Response);
    }
};


void setup() {
  responseGenerator(); // Gen CapPort resp

  //your other setup stuff...
  WiFi.softAP("esp-captive");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  //more handlers...
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
