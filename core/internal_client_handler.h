#ifndef NDNFD_CORE_INTERNAL_CLIENT_HANDLER_H_
#define NDNFD_CORE_INTERNAL_CLIENT_HANDLER_H_
#include "core/element.h"
extern "C" {
#include <ccn/ccn.h>
#define WANT_NDNFD_INTERNAL_CLIENT_OPS
#include "ccnd/ndnfd_internal_client.h"
}
#include "util/buffer.h"
namespace ndnfd {


class InternalClientHandler : public Element {
 public:
  // ResponseKind indicates what response is sent for the Interest
  enum class ResponseKind {
    kRespond = 0,//respond with Content
    kSilent  = -1,//don't respond
    kNack    = CCN_CONTENT_NACK//respond with Nack
  };
  
  InternalClientHandler(void) {}
  virtual ~InternalClientHandler(void) {}
  
  // Req* processes a kind of request.
  // msg is the last component of the Name in Interest; size is the length of msg.
  // It returns ResponseKind, and the payload (Content) in a response.
  
  std::tuple<ResponseKind,Ptr<Buffer>> ReqSignature(const uint8_t* msg, size_t size);
  std::tuple<ResponseKind,Ptr<Buffer>> ReqNewFace(const uint8_t* msg, size_t size);
  std::tuple<ResponseKind,Ptr<Buffer>> ReqDestroyFace(const uint8_t* msg, size_t size);
  std::tuple<ResponseKind,Ptr<Buffer>> ReqStats(const uint8_t* msg, size_t size);
  std::tuple<ResponseKind,Ptr<Buffer>> ReqListStrategy(const uint8_t* msg, size_t size);
  std::tuple<ResponseKind,Ptr<Buffer>> ReqSetStrategy(const uint8_t* msg, size_t size);

 private:
  DISALLOW_COPY_AND_ASSIGN(InternalClientHandler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_INTERNAL_CLIENT_HANDLER_H_
