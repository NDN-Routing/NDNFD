#include "defs.h"
#include "gtest/gtest.h"
namespace ndnfd {

class RefCountTestObj : public Object {
 public:
  static int n;
  RefCountTestObj(void) { ++RefCountTestObj::n; }
  virtual ~RefCountTestObj(void) { --RefCountTestObj::n; }
  
 private:
  DISALLOW_COPY_AND_ASSIGN(RefCountTestObj);
};
int RefCountTestObj::n = 0;

TEST(UtilTest, RefCountObj) {
  Ptr<RefCountTestObj> p, q;
  EXPECT_EQ(0, RefCountTestObj::n);
  p = new RefCountTestObj();
  EXPECT_EQ(1, RefCountTestObj::n);
  q = p;
  EXPECT_EQ(1, RefCountTestObj::n);
  p = nullptr;
  EXPECT_EQ(1, RefCountTestObj::n);
  q = nullptr;
  EXPECT_EQ(0, RefCountTestObj::n);
}

};//namespace ndnfd
