#ifndef NDNFD_NS3_MODEL_NDNFDSIM_H_
#define NDNFD_NS3_MODEL_NDNFDSIM_H_
#include "core/element.h"
#include "face/internal_client.h"
namespace ndnfd {

class NdnfdSim : public Element {
 public:
  NdnfdSim(uint32_t nodeid) { this->nodeid_ = nodeid; }
  virtual void Init(void);
  virtual ~NdnfdSim(void) {}
  
  void Start(void);
  
 private:
  uint32_t nodeid_;
  Ptr<InternalClientFace> internal_client_;
  
  static void CcndGetTime(const ccn_gettime* self, ccn_timeval* result);
  void RunOnce(void);
  
  DISALLOW_COPY_AND_ASSIGN(NdnfdSim);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_NDNFDSIM_H_
