#include "logging.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
namespace ndnfd {

NS_LOG_COMPONENT_DEFINE("NDNFD");

SimLogging::SimLogging(uint32_t nodeid) : nodeid_(nodeid) {
  g_log.Disable(ns3::LOG_PREFIX_FUNC);
  g_log.Disable(ns3::LOG_PREFIX_LEVEL);
}

void SimLogging::WriteLine(LoggingLevel level, LoggingComponent component, const char* s) {
  ns3::LogLevel ll = ns3::LOG_NONE;
  switch (level) {
    case kLLDebug: ll = ns3::LOG_DEBUG; break;
    case kLLInfo:  ll = ns3::LOG_INFO ; break;
    case kLLWarn:  ll = ns3::LOG_WARN ; break;
    case kLLError: ll = ns3::LOG_ERROR; break;
  }
  NS_LOG(ll, s);
}

};//namespace ndnfd
