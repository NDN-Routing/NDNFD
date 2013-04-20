#ifndef NDNFD_NS3_MODEL_MOCK_FIB_H_
#define NDNFD_NS3_MODEL_MOCK_FIB_H_
#include <ns3/ndn-fib.h>
#include "global.h"
namespace ndnfd {

class MockFib : public ns3::ndn::Fib {
 public:
  static ns3::TypeId GetTypeId(void);
  explicit MockFib(SimGlobal* global) { this->global_ = global; }
  virtual ~MockFib(void) {}

  virtual ns3::Ptr<ns3::ndn::fib::Entry> LongestPrefixMatch(const ns3::ndn::Interest& interest) { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<ns3::ndn::fib::Entry> Find(const ns3::ndn::Name& prefix) { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<ns3::ndn::fib::Entry> Add(const ns3::ndn::Name& prefix, ns3::Ptr<ns3::ndn::Face> face, int32_t metric);
  virtual ns3::Ptr<ns3::ndn::fib::Entry> Add(const ns3::Ptr<const ns3::ndn::Name>& prefix, ns3::Ptr<ns3::ndn::Face> face, int32_t metric);
  virtual void Remove(const ns3::Ptr<const ns3::ndn::Name>& prefix) { NS_ASSERT(false); }
  virtual void InvalidateAll(void) { NS_ASSERT(false); }
  virtual void RemoveFromAll(ns3::Ptr<ns3::ndn::Face> face) { NS_ASSERT(false); }
  virtual void Print(std::ostream& os) const {}
  virtual uint32_t GetSize(void) const { NS_ASSERT(false); return 0; }
  virtual ns3::Ptr<const ns3::ndn::fib::Entry> Begin(void) const { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<ns3::ndn::fib::Entry> Begin(void) { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<const ns3::ndn::fib::Entry> End(void) const { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<ns3::ndn::fib::Entry> End(void) { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<const ns3::ndn::fib::Entry> Next(ns3::Ptr<const ns3::ndn::fib::Entry>) const { NS_ASSERT(false); return nullptr; }
  virtual ns3::Ptr<ns3::ndn::fib::Entry> Next(ns3::Ptr<ns3::ndn::fib::Entry>) { NS_ASSERT(false); return nullptr; }
 
 private:
  SimGlobal* global_;
  SimGlobal* global(void) const { NS_ASSERT(this->global_ != nullptr); return this->global_; }
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_MOCK_FIB_H_
