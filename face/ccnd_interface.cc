#include "ccnd_interface.h"
#include "face/facemgr.h"
using namespace ndnfd;

struct face* face_from_faceid(struct ccnd_handle *h, unsigned faceid) {
  Global* global = ccnd_ndnfdGlobal(h);
  Ptr<Face> face = global->facemgr()->GetFace(static_cast<FaceId>(faceid));
  if (face == nullptr) return nullptr;
  return face->ccnd_face();
}

