#ifndef NDNFD_NS3_UTILS_SIMCLOCK_H_
#define NDNFD_NS3_UTILS_SIMCLOCK_H_
#include "util/defs.h"
#include <ns3/nstime.h>
#include <ns3/simulator.h>
namespace ndnfd {

class SimClock {
 public:
  typedef std::chrono::nanoseconds duration;
  typedef duration::rep rep;
  typedef duration::period period;
  typedef std::chrono::time_point<SimClock> time_point;
  static const bool is_steady = true;
  static time_point now(void) { ns3::Time t = ns3::Now(); return time_point(duration(t.GetNanoSeconds())); }
};

};//namespace ndnfd
#endif//NDNFD_NS3_UTILS_SIMCLOCK_H_
