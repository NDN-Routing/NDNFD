#ifndef NDNFD_NS3_MODEL_GLOBAL_H_
#define NDNFD_NS3_MODEL_GLOBAL_H_
#include "core/global.h"
#include <ns3/ptr.h>
namespace ndnfd {

class L3Protocol;
class NdnfdSim;
class NdnsimPacketConverter;

// A SimGlobal contains all global structures of a router in ns3.
class SimGlobal : public Global {
 public:
  SimGlobal(uint32_t nodeid);
  virtual void Init(void);
  virtual ~SimGlobal(void);
  
  uint32_t nodeid() const { return this->nodeid_; }
  L3Protocol* l3(void) const { return this->l3_; }
  void set_l3(ns3::Ptr<L3Protocol> value);
  NdnfdSim* program(void) const { return this->program_; }
  void set_program(Ptr<NdnfdSim> value);
  NdnsimPacketConverter* npc(void) const { return this->npc_; }
  void set_npc(Ptr<NdnsimPacketConverter> value);
  
 private:
  uint32_t nodeid_;
  L3Protocol* l3_;
  NdnfdSim* program_;
  NdnsimPacketConverter* npc_;

  DISALLOW_COPY_AND_ASSIGN(SimGlobal);
};

#define THIS_SIMGLOBAL (static_cast<SimGlobal*>(this->global()))

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_GLOBAL_H_
