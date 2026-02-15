#include "TailSyncWebConfig.h"
#include "generated/index.htm.h"
#include <ESPmDNS.h>
#include <Logging/TailSyncLogging.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

const char *WebConfigHost = "tailsync";
const char *WebConfigSSID = "tailsync";
const char *WebConfigPassword = "tailsync";

static Logger logger("TailSync Web Config");
bool WebServerIsInit = false;
WebServer *server = nullptr;

void WebConfigInit() {
  server = new WebServer(80);
  // Connect to Wi-Fi network
  WiFiClass::mode(WIFI_MODE_STA);
  WiFi.begin(WebConfigSSID, WebConfigPassword);
  logger.log(DEBUG, "Attempting to connect...");
  while (WiFiClass::status() == WL_DISCONNECTED) {
    logger.log(DEBUG, "Waiting to connect...");
    delay(500);
  }
  // if we can't find the network
  if (WiFiClass::status() == WL_NO_SSID_AVAIL) {
    logger.log(WARNING, "Failed to connect! Hosting instead");
    WiFiClass::mode(WIFI_MODE_AP);
    WiFi.softAP(WebConfigSSID, WebConfigPassword);
    while (WiFiClass::status() == WL_DISCONNECTED) {
      logger.log(DEBUG, "Waiting for network to come up...");
      delay(500);
    }
    logger.log(DEBUG, "Hosting (AP mode) %s, IP: %s", WebConfigHost,
               WiFi.softAPIP().toString().c_str());
  } else {
    logger.log(DEBUG, "Connected to %s, IP: %s", WebConfigHost,
               WiFi.localIP().toString().c_str());
  }

  if (!MDNS.begin(WebConfigHost)) {
    logger.log(ERROR, "Failed to set up MDNS!");
  }
  server->on("/", HTTP_GET,
             []() { server->send(200, "text/html", index_htm); });

  server->on("/reboot", HTTP_POST, []() {
    logger.log(DEBUG, "Rebooting...");
    server->send(200, "text/plain", "Rebooting...");
    delay(100);
    ESP.restart();
  });
  /*handling uploading firmware file */
  server->on(
      "/update", HTTP_POST,
      []() {
        server->sendHeader("Connection", "close");
        server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      },
      []() {
        HTTPUpload &upload = server->upload();
        if (upload.status == UPLOAD_FILE_START) {
          logger.log(DEBUG, "Firmware blob being uploaded: %s",
                     upload.filename.c_str());
          if (!Update.begin(
                  UPDATE_SIZE_UNKNOWN)) { // start with max available size
            logger.log(ERROR, Update.errorString());
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            logger.log(ERROR, Update.errorString());
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            logger.log(DEBUG, "Update completed successfully! Rebooting...");
          } else {
            logger.log(ERROR, Update.errorString());
          }
        }
      });
  server->begin();
  WebServerIsInit = true;
}

void WebConfig_tick() {
  if (!WebServerIsInit) {
    WebConfigInit();
  }
  server->handleClient();
  delay(1);
}
void destroy() { ESP.restart(); }
