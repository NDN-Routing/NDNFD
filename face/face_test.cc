#include "face.h"
#include "gtest/gtest.h"
#include "core/element_testh.h"
namespace ndnfd {

class FaceTestFace : public Face {
 public:
  FaceTestFace(void) {}
 private:
  DISALLOW_COPY_AND_ASSIGN(FaceTestFace);
};

TEST(FaceTest, Face) {
  Ptr<FaceTestFace> face = NewTestElement<FaceTestFace>();
  // TODO test face->Enroll() when FaceMgr is ready
}

};//namespace ndnfd
