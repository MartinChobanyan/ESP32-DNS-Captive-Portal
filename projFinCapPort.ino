#include <SPIFFS.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

File data;
String ssid, pass;
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

bool connectToAP(String ssid = ::ssid, String pass = ::pass) {
  wifi_mode_t returnMode = WiFi.getMode();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pass.c_str());
  delay(1000);
  if (WiFi.status() != WL_CONNECTED) delay(1000);
  if (WiFi.status() == WL_CONNECTED) return true;
  else {
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
          ::ssid = p1->value();
          ::pass = p2->value();
          if (connectToAP()) {
            dnsServer.stop();
            if (!SPIFFS.begin(true)) {
              Serial.println("An Error has occurred while mounting SPIFFS");
              return;
            }

            data = SPIFFS.open("/data.txt", "w +");

            data.println(::ssid + ' ' + ::pass);

            data.close();
            SPIFFS.end();
            return;
          } else return;
        }
      }

      request->send(200, "text/html", ::Response);
    }
};


void setup() {
  WiFi.mode(WIFI_STA);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  data = SPIFFS.open("/data.txt", "w +");

  if (data) {
    while (data.available()) {
      char t = data.read();
      if (t != ' ') {
        ssid += t;
      } else {
        pass += t;
      }
    }
    data.close();
    SPIFFS.end();
    if (connectToAP()) {
      return;
    }
  } else {
    responseGenerator(); // Gen CapPort resp
  }

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
