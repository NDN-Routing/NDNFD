#ifndef NDNFD_NS3_MODEL_FACEMGR_H_
#define NDNFD_NS3_MODEL_FACEMGR_H_
#include "face/facemgr.h"
namespace ndnfd {

class FaceMgrSim : public FaceMgr {
 public:
  FaceMgrSim(void) {}
  virtual ~FaceMgrSim(void) {}
  
  virtual void StartDefaultListeners(void);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(FaceMgrSim);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_FACEMGR_H_
