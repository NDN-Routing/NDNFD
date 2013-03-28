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
  
  char buf[512];
  int res = vsnprintf(buf, sizeof(buf), format, args);
  if (res <= 0) return;
  
  size_t len = strlen(buf);
  if (buf[len-1] == '\n') buf[len-1] = '\0';
  
  this->WriteLine(level, component, buf);
}

std::string Logging::ErrorString(int errnum) {
  return std::string(strerror(errno));
}

void Logging::WriteLine(LoggingLevel level, LoggingComponent component, const char* s) {
  fprintf(stderr, "%s\n", s);
}

int Logging::CcndLogger(void* loggerdata, const char* format, va_list ap) {
  Logging* logging = static_cast<Logging*>(loggerdata);
  logging->LogVA(kLLInfo, kLCCcndCore, format, ap);
  return 1;
}

};//namespace ndnfd
