#ifndef NDNFD_NS3_MODEL_L3PROTOCOL_H_
#define NDNFD_NS3_MODEL_L3PROTOCOL_H_
#include "ns3/ndn-l3-protocol.h"
#include "face/faceid.h"
#include "message/message.h"

namespace ns3 {
namespace ndnfd {

class L3Protocol : public ns3::ndn::L3Protocol {
 public:
  static ns3::TypeId GetTypeId(void);
  L3Protocol(void) {}
  virtual ~L3Protocol(void) {}

  // AddFace registers an AppFace.
  virtual uint32_t AddFace(const ns3::Ptr<ns3::ndn::Face> face);
  
  // RemoveFace deletes a Face.
  virtual void RemoveFace(ns3::Ptr<ns3::ndn::Face> face);
  
  // GetFaceByNetDevice is not supported because lower layer is provided by NDNFD core.
  virtual ns3::Ptr<ns3::ndn::Face> GetFaceByNetDevice(ns3::Ptr<ns3::NetDevice> netDevice) const { return nullptr; }

 protected:
  virtual void NotifyNewAggregate(void);

 private:
  // AppReceive receives a message from AppFace, and pass it to NDNFD.
  void AppReceive(const ns3::Ptr<ns3::ndn::Face>& face, const ns3::Ptr<const ns3::Packet>& p);
  
  // AppSend delivers a message to AppFace.
  void AppDeliver(::ndnfd::FaceId faceid, const ::ndnfd::Ptr< ::ndnfd::Message> msg);

  DISALLOW_COPY_AND_ASSIGN(L3Protocol);
};

};//namespace ndnfd
};//namespace ns3
#endif//NDNFD_NS3_MODEL_L3PROTOCOL_H_
