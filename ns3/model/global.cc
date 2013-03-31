#include "global.h"
#include "l3protocol.h"
#include "../utils/logging.h"
#include "facemgr.h"
#include "ndnfdsim.h"
namespace ndnfd {

SimGlobal::SimGlobal(uint32_t nodeid) : nodeid_(nodeid), l3_(nullptr), program_(nullptr), npc_(nullptr) {}

void SimGlobal::Init(void) {
  this->Global::Init();
  Ptr<Element> first = Element::MakeFirstElement(this);
  this->set_logging(new SimLogging(this->nodeid_));
  this->set_facemgr(first->New<FaceMgrSim>());
  this->set_npc(new NdnsimPacketConverter());
}

SimGlobal::~SimGlobal(void) {
  this->set_l3(nullptr);
  this->set_program(nullptr);
  this->set_npc(nullptr);
}

void SimGlobal::set_l3(ns3::Ptr<L3Protocol> value) {
  if (this->l3_ != nullptr) {
    this->l3_->Unref();
  }
  this->l3_ = ns3::GetPointer(value);
}

void SimGlobal::set_program(Ptr<NdnfdSim> value) {
  if (this->program_ != nullptr) {
    this->program_->Unref();
  }
  this->program_ = GetPointer(value);
}

void SimGlobal::set_npc(Ptr<NdnsimPacketConverter> value) {
  if (this->npc_ != nullptr) {
    this->npc_->Unref();
  }
  this->npc_ = GetPointer(value);
}

};//namespace ndnfd
