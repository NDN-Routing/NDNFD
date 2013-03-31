#ifndef NDNFD_NS3_MODEL_NDNFDSIM_H_
#define NDNFD_NS3_MODEL_NDNFDSIM_H_
#include <queue>
#include <ns3/simulator.h>
#include "core/element.h"
#include "face/internal_client.h"
namespace ndnfd {

class NdnfdSim : public Element {
 public:
  NdnfdSim(void) {}
  virtual void Init(void);
  virtual ~NdnfdSim(void);
  
  void Start(void);
  void ScheduleOnNextRun(std::function<void()> action);
  
 private:
  Ptr<InternalClientFace> internal_client_;
  std::queue<std::function<void()>> next_run_actions_;
  
  static void CcndGetTime(const ccn_gettime* self, ccn_timeval* result);
  void RunOnce(void);
  
  DISALLOW_COPY_AND_ASSIGN(NdnfdSim);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_NDNFDSIM_H_
