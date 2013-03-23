#include "internal_client_handler.h"
#include "face/faceid.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}

#define NDNFD_DEF_REQ(op,method) \
int ndnfd_req_##op(ccnd_handle* h, const uint8_t* msg, size_t size, ccn_charbuf* reply_body) { \
  ndnfd::Global* global = ccnd_ndnfdGlobal(h); \
  ndnfd::InternalClientHandler::ResponseKind res; std::string reply; \
  std::tie(res, reply) = global->internal_client_handler()->method(msg, size); \
  if (reply_body != nullptr) ccn_charbuf_append(reply_body, reply.data(), reply.size()); \
  return static_cast<int>(res); \
}
NDNFD_DEF_REQ(signature,ReqSignature);
NDNFD_DEF_REQ(newface,ReqNewFace);
NDNFD_DEF_REQ(destroyface,ReqDestroyFace);

namespace ndnfd {

std::tuple<InternalClientHandler::ResponseKind,std::string> InternalClientHandler::ReqSignature(const uint8_t* msg, size_t size) {
  return std::forward_as_tuple(ResponseKind::kRespond, "NDNFD/20130322");
}

std::tuple<InternalClientHandler::ResponseKind,std::string> InternalClientHandler::ReqNewFace(const uint8_t* msg, size_t size) {
  FaceId inface = static_cast<FaceId>(this->global()->ccndh()->interest_faceid);
  this->Log(kLLDebug, kLCIntClientH, "InternalClientHandler::ReqNewFace(msg,%" PRIuMAX ") inface=%" PRI_FaceId "", (uintmax_t)size, inface);
  return std::forward_as_tuple(ResponseKind::kSilent, "");
}

std::tuple<InternalClientHandler::ResponseKind,std::string> InternalClientHandler::ReqDestroyFace(const uint8_t* msg, size_t size) {
  FaceId inface = static_cast<FaceId>(this->global()->ccndh()->interest_faceid);
  this->Log(kLLDebug, kLCIntClientH, "InternalClientHandler::ReqDestroyFace(msg,%" PRIuMAX ") inface=%" PRI_FaceId "", (uintmax_t)size, inface);
  return std::forward_as_tuple(ResponseKind::kSilent, "");
}


};//namespace ndnfd
