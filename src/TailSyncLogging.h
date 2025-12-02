#include <string>

enum logLevel { FATAL = 0, CRITICAL = 1, ERROR = 2, WARNING = 3, DEBUG = 4 };

class Logger {
  const char *moduleName;

public:
  Logger(const char *modName) : moduleName(modName) {};
  void log(logLevel level, const char *fmt, ...);
};
