#ifndef NDNFD_NS3_UTILS_PKTCONV_H_
#define NDNFD_NS3_UTILS_PKTCONV_H_
#include "message/interest.h"
#include "message/contentobject.h"
#include "message/nack.h"
#include <ns3/ndn-interest.h>
#include <ns3/ndn-content-object.h>
namespace ndnfd {

// NdnsimPacketConverter converts between CcnbMessage and ndnSIM entities.
//   This is designed as a plain converter rather than a WireProtocol,
//   so that we can operate on ns3::ndn::* without going through ndnfd::Buffer.
class NdnsimPacketConverter : public Object {
 public:
  NdnsimPacketConverter(void) : s_(0) {}
  virtual ~NdnsimPacketConverter(void) {}
  
  Ptr<Name> NameFrom(ns3::Ptr<const ns3::ndn::Name> name) const;
  Ptr<InterestMessage> InterestFrom(ns3::Ptr<const ns3::ndn::Interest> interest) const;
  Ptr<ContentObjectMessage> ContentObjectFrom(ns3::Ptr<const ns3::ndn::ContentObject> co) const;
  NackCode NackCodeFrom(uint8_t code) const;
  Ptr<NackMessage> NackFrom(ns3::Ptr<const ns3::ndn::Interest> nack) const;

  ns3::Ptr<ns3::ndn::Name> NameTo(Ptr<const Name> name) const;
  ns3::Ptr<ns3::ndn::Interest> InterestTo(Ptr<const InterestMessage> msg) const;
  ns3::Ptr<ns3::ndn::ContentObject> ContentObjectTo(Ptr<const ContentObjectMessage> msg) const;
  uint8_t NackCodeTo(NackCode code) const;
  ns3::Ptr<ns3::ndn::Interest> NackTo(Ptr<const NackMessage> msg) const;
  
 private:
  uint64_t s_;
  DISALLOW_COPY_AND_ASSIGN(NdnsimPacketConverter);
};

};//namespace ndnfd
#endif//NDNFD_NS3_UTILS_PKTCONV_H_
