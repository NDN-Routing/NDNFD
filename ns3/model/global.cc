#include "global.h"
#include "l3protocol.h"
#include "facemgr.h"
namespace ndnfd {

SimGlobal::SimGlobal(void) {
  this->l3_ = nullptr;
}

void SimGlobal::Init(void) {
  this->Global::Init();
  Ptr<Element> first = Element::MakeFirstElement(this);
  this->set_facemgr(first->New<FaceMgrSim>());
}

SimGlobal::~SimGlobal(void) {
  this->set_l3(nullptr);
}

void SimGlobal::set_l3(ns3::Ptr<L3Protocol> value) {
  if (this->l3_ != nullptr) {
    this->l3_->set_global(nullptr);
    this->l3_->Unref();
  }
  this->l3_ = ns3::GetPointer(value);
  if (this->l3_ != nullptr) {
    this->l3_->set_global(this);
  }
}

};//namespace ndnfd
