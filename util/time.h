#ifndef NDNFD_UTIL_TIME_H_
#define NDNFD_UTIL_TIME_H_
#include "util/defs.h"
#include <time.h>
namespace ndnfd {

// DateTime represents a time point.
// DateTime is stored as nanoseconds since UNIX epoch.
typedef uint64_t DateTime;
// the UNIX epoch
const DateTime DateTime_epoch = 0;
// an invalid time point
const DateTime DateTime_invalid = UINT64_C(0xFFFFFFFFFFFFFFFF);

// TimeSpan represents length of a time period.
// TimeSpan is stored as nanoseconds.
typedef int64_t TimeSpan;
// zero
const TimeSpan TimeSpan_zero = 0;
// one second
const TimeSpan TimeSpan_second = 1000000000;

// DateTime_now returns current time.
DateTime DateTime_now(void);
// DateTime_make creates DateTime from timespec.
inline DateTime DateTime_make(struct timespec ts) { return ts.tv_sec * TimeSpan_second + ts.tv_nsec; }
// DateTime_time converts DateTime to time_t.
inline time_t DateTime_time(DateTime self) { return self / TimeSpan_second; }

// DateTime_diff calculates difference between two times (other - self).
inline TimeSpan DateTime_diff(DateTime self, DateTime other) { return static_cast<TimeSpan>(other - self); }
// DateTime_add calculates the time after a time period.
inline DateTime DateTime_add(DateTime self, TimeSpan diff) { return static_cast<DateTime>(self + diff); }

};//namespace ndnfd
#endif//NDNFD_UTIL_TIME_H_
