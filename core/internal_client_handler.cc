#include "internal_client_handler.h"
#include "face/faceid.h"
#include "face/facemgr.h"
extern "C" {
#include "ccnd/ccnd_private.h"
struct ccn_charbuf* collect_stats_xml(struct ccnd_handle* h);
}

#define NDNFD_DEF_REQ(op,method) \
int ndnfd_req_##op(ccnd_handle* h, const uint8_t* msg, size_t size, ccn_charbuf* reply_body) { \
  ndnfd::Global* global = ccnd_ndnfdGlobal(h); \
  ndnfd::InternalClientHandler::ResponseKind res; ndnfd::Ptr<ndnfd::Buffer> reply; \
  std::tie(res, reply) = global->internal_client_handler()->method(msg, size); \
  if (reply_body != nullptr && reply != nullptr) reply->Swap(reply_body); \
  return static_cast<int>(res); \
}
NDNFD_DEF_REQ(signature,ReqSignature);
NDNFD_DEF_REQ(newface,ReqNewFace);
NDNFD_DEF_REQ(destroyface,ReqDestroyFace);
NDNFD_DEF_REQ(stats,ReqStats);

namespace ndnfd {

std::tuple<InternalClientHandler::ResponseKind,Ptr<Buffer>> InternalClientHandler::ReqSignature(const uint8_t* msg, size_t size) {
  static std::string reply_str = "NDNFD/20130322";
  Ptr<Buffer> reply = new Buffer(reply_str.size());
  memcpy(reply->mutable_data(), reply_str.data(), reply->length());
  return std::forward_as_tuple(InternalClientHandler::ResponseKind::kRespond, reply);
}

std::tuple<InternalClientHandler::ResponseKind,Ptr<Buffer>> InternalClientHandler::ReqNewFace(const uint8_t* msg, size_t size) {
  FaceId inface = static_cast<FaceId>(CCNDH->interest_faceid);
  this->Log(kLLDebug, kLCIntClientH, "InternalClientHandler::ReqNewFace(msg,%" PRIuMAX ") inface=%" PRI_FaceId "", (uintmax_t)size, inface);
  return this->global()->facemgr()->FaceMgmtReq(FaceMgr::FaceMgmtProtoAct::kNewFace, inface, msg, size);
}

std::tuple<InternalClientHandler::ResponseKind,Ptr<Buffer>> InternalClientHandler::ReqDestroyFace(const uint8_t* msg, size_t size) {
  FaceId inface = static_cast<FaceId>(CCNDH->interest_faceid);
  this->Log(kLLDebug, kLCIntClientH, "InternalClientHandler::ReqDestroyFace(msg,%" PRIuMAX ") inface=%" PRI_FaceId "", (uintmax_t)size, inface);
  return this->global()->facemgr()->FaceMgmtReq(FaceMgr::FaceMgmtProtoAct::kDestroyFace, inface, msg, size);
}

std::tuple<InternalClientHandler::ResponseKind,Ptr<Buffer>> InternalClientHandler::ReqStats(const uint8_t* msg, size_t size) {
  ccn_charbuf* html = collect_stats_xml(CCNDH);
  Ptr<Buffer> reply = new Buffer(0);
  reply->Swap(html);
  ccn_charbuf_destroy(&html);
  return std::forward_as_tuple(InternalClientHandler::ResponseKind::kRespond, reply);
}

};//namespace ndnfd
