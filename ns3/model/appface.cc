#include "appface.h"
#include "face/facemgr.h"
namespace ndnfd {

void SimAppFace::Init(void) {
  this->set_kind(FaceKind::kApp);
  this->global()->facemgr()->AddFace(this);
}

};//namespace ndnfd
