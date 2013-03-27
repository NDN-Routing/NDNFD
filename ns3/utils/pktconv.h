#ifndef NDNFD_NS3_UTILS_PKTCONV_H_
#define NDNFD_NS3_UTILS_PKTCONV_H_
#include "message/interest.h"
#include "message/contentobject.h"
#include <ns3/ndn-interest.h>
#include <ns3/ndn-content-object.h>
namespace ndnfd {

// NdnsimPacketConverter converts between CcnbMessage and ns3::Packet in ndnSIM format.
//   This is designed as a plain converter rather than a WireProtocol,
//   so that we can operate on ns3::Packet without going through ndnfd::Buffer.
class NdnsimPacketConverter : public Object {
 public:
  NdnsimPacketConverter(void) {}
  virtual ~NdnsimPacketConverter(void) {}
  
  Ptr<CcnbMessage> MessageFrom(ns3::Ptr<ns3::Packet> p);
  Ptr<Name> NameFrom(const ns3::ndn::Name& name);
  Ptr<InterestMessage> InterestFrom(const ns3::ndn::Interest& header);
  Ptr<ContentObjectMessage> ContentObjectFrom(const ns3::ndn::ContentObject& header, ns3::Ptr<ns3::Packet> payload);

  ns3::Ptr<ns3::Packet> MessageTo(Ptr<const Message> msg);
  ns3::Ptr<ns3::ndn::Name> NameTo(Ptr<const Name> name);
  ns3::Ptr<ns3::Packet> InterestTo(Ptr<const InterestMessage> msg);
  ns3::Ptr<ns3::Packet> ContentObjectTo(Ptr<const ContentObjectMessage> msg);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(NdnsimPacketConverter);
};

};//namespace ndnfd
#endif//NDNFD_NS3_UTILS_PKTCONV_H_
