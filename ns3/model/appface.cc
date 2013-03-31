#include "appface.h"
#include "face/facemgr.h"
namespace ndnfd {

void SimAppFace::Init(void) {
  this->global()->facemgr()->AddFace(this);
}

};//namespace ndnfd
