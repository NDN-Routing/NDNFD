#ifndef NDNFD_COMMAND_NDNFD_H_
#define NDNFD_COMMAND_NDNFD_H_
#include "core/element.h"
#include "face/internal_client.h"
#include "face/unix.h"
#include "face/ip.h"
namespace ndnfd {

class NdnfdProgram : public Element {
 public:
  NdnfdProgram(int argc, char** argv) {}
  virtual void Init(void);
  virtual ~NdnfdProgram(void) {}
  void Run(void);
  
 private:
  Ptr<InternalClientFace> internal_client_;
  Ptr<UnixFaceFactory> unix_face_factory_;
  Ptr<StreamListener> unix_listener_;
  Ptr<TcpFaceFactory> tcp_face_factory_;
  Ptr<UdpFaceFactory> udp_face_factory_;
  Ptr<DgramChannel> udp_channel_;
  
  void ccndc_add(FaceId faceid, std::string prefix);
  
  DISALLOW_COPY_AND_ASSIGN(NdnfdProgram);
};

int CcndLogger(void* loggerdata, const char* format, va_list ap);


};//namespace ndnfd
#endif//NDNFD_UTIL_BUFFER_H
