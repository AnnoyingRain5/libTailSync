#include "TailSyncLogging.h"
#include "Arduino.h"

const char *getName(logLevel level) {
  switch (level) {
  case FATAL:
    return "Fatal";
  case CRITICAL:
    return "Critical";
  case ERROR:
    return "Error";
  case WARNING:
    return "Warning";
  case DEBUG:
    return "Debug";
  default:
    return "Unknown (!!!!)";
  }
}

void Logger::log(logLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char *buf = nullptr;
  if (vasprintf(&buf, fmt, args) < 0) {
    va_end(args);
    return;
  }
  va_end(args);
  Serial.printf("[%s] [%s] %s\n", moduleName, getName(level), buf);
  free(buf);
}
