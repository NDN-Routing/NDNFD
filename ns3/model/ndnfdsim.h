#ifndef NDNFD_NS3_MODEL_NDNFDSIM_H_
#define NDNFD_NS3_MODEL_NDNFDSIM_H_
#include "core/element.h"
#include "face/internal_client.h"
#include "../utils/simclock.h"
namespace ndnfd {

class NdnfdSim : public Element {
 public:
  NdnfdSim(uint32_t nodeid) { this->nodeid_ = nodeid; }
  virtual void Init(void);
  virtual ~NdnfdSim(void) {}
  SimClock::duration RunOnce(void);
  
 private:
  uint32_t nodeid_;
  Ptr<InternalClientFace> internal_client_;
  
  DISALLOW_COPY_AND_ASSIGN(NdnfdSim);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_NDNFDSIM_H_
