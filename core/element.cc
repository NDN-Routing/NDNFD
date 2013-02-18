#include "element.h"
#include <cstdarg>
namespace ndnfd {

Ptr<Element> Element::MakeFirstElement(Global* global) {
  Ptr<Element> ele = new Element();
  ele->global_ = global;
  return ele;
}

void Element::Log(LoggingLevel level, LoggingComponent component, const char* format, ...) const {
  va_list args;
  va_start(args, format);
  this->global()->logging()->LogVA(level, component, format, &args);
  va_end(args);
}

};//namespace ndnfd
