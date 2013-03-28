#ifndef NDNFD_COMMAND_NDNFD_H_
#define NDNFD_COMMAND_NDNFD_H_
#include "core/element.h"
#include "face/internal_client.h"
namespace ndnfd {

class NdnfdProgram : public Element {
 public:
  NdnfdProgram(int argc, char** argv) {}
  virtual void Init(void);
  virtual ~NdnfdProgram(void) {}
  void Run(void);
  
 private:
  Ptr<InternalClientFace> internal_client_;
  
  void ccndc_add(FaceId faceid, std::string prefix);
  
  DISALLOW_COPY_AND_ASSIGN(NdnfdProgram);
};

};//namespace ndnfd
#endif//NDNFD_UTIL_BUFFER_H
