#include "util/time.h"
#include <unistd.h>
#include "gtest/gtest.h"
namespace ndnfd {

TEST(UtilTest, DateTime) {
  DateTime dt1 = DateTime_now();
  ::usleep(1);
  DateTime dt2 = DateTime_now();
  time_t now2 = ::time(NULL);
  
  ASSERT_GE(DateTime_diff(dt1, dt2), (TimeSpan)1000);
  ASSERT_LE(now2 - DateTime_time(dt2), 1);
  ASSERT_EQ(dt2, DateTime_add(dt1, DateTime_diff(dt1, dt2)));
  ASSERT_EQ(dt1, DateTime_add(dt2, DateTime_diff(dt2, dt1)));
}

};//namespace ndnfd
