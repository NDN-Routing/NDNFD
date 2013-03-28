#include "util/logging.h"
#include <cstdio>
#include <cstdarg>
namespace ndnfd {

Logging::Logging(void) {
  this->min_level_ = kLLDebug;
  this->components_ = ~0;
}

void Logging::Log(LoggingLevel level, LoggingComponent component, const char* format, ...) {
  va_list args;
  va_start(args, format);
  this->LogVA(level, component, format, args);
  va_end(args);
}

void Logging::LogVA(LoggingLevel level, LoggingComponent component, const char* format, va_list args) {
  if (level < Logging::min_level()) return;
  if ((component & Logging::components()) == 0) return;
  
  fprintf(stderr, "%s", this->line_prefix().c_str());
  vfprintf(stderr, format, args);
  if (format[0] == '\0' || format[strlen(format)-1] != '\n') fprintf(stderr, "\n");
}

std::string Logging::ErrorString(int errnum) {
  return std::string(strerror(errno));
}

int Logging::CcndLogger(void* loggerdata, const char* format, va_list ap) {
  Logging* logging = static_cast<Logging*>(loggerdata);
  logging->LogVA(kLLInfo, kLCCcndCore, format, ap);
  return 1;
}

};//namespace ndnfd
