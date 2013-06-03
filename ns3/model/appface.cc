#include "appface.h"
#include "face/facemgr.h"
namespace ndnfd {

void SimAppFace::Init(void) {
  this->set_kind(FaceKind::kApp);
  this->global()->facemgr()->AddFace(this);
}

FaceDescription SimAppFace::GetDescription(void) const {
  FaceDescription d;
  d.proto_ = "NDNSIM";
  return d;
}

};//namespace ndnfd
