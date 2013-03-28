#include "logging.h"
#include <ns3/simulator.h>
namespace ndnfd {

std::string SimLogging::line_prefix(void) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%f [%" PRIu32 "] ", ns3::Now().GetSeconds(), this->nodeid_);
  return buf;
}

};//namespace ndnfd
