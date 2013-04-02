#include "foreach.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(UtilTest, Foreach) {
  auto f1 = [] { FOREACH_BREAK; };
  EXPECT_TRUE(ForeachAction_break(f1()));
  auto f2 = [] { FOREACH_OK; };
  EXPECT_FALSE(ForeachAction_break(f2()));
}

};//namespace ndnfd
