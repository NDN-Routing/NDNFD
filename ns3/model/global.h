#ifndef NDNFD_NS3_MODEL_GLOBAL_H_
#define NDNFD_NS3_MODEL_GLOBAL_H_
#include "core/global.h"
#include <ns3/ptr.h>
namespace ndnfd {

class L3Protocol;

// A SimGlobal contains all global structures of a router in ns3.
class SimGlobal : public Global {
 public:
  SimGlobal(void);
  virtual void Init(void);
  virtual ~SimGlobal(void);

  L3Protocol* l3(void) const { return this->l3_; }
  void set_l3(ns3::Ptr<L3Protocol> value);
  
 private:
  L3Protocol* l3_;
  DISALLOW_COPY_AND_ASSIGN(SimGlobal);
};

#define THIS_SIMGLOBAL (static_cast<SimGlobal*>(this->global()))

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_GLOBAL_H_
