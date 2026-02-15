#pragma once
#include <WebServer.h>

extern WebServer *server;
extern const char *WebConfigHost;
extern const char *WebConfigSSID;
extern const char *WebConfigPassword;
void WebConfig_tick();