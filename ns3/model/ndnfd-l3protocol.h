#ifndef NDNFD_NS3_MODEL_L3PROTOCOL_H_
#define NDNFD_NS3_MODEL_L3PROTOCOL_H_
#include <map>
#include <ns3/node.h>
#include <ns3/ndn-l3-protocol.h>
#include <ns3/ndn-face.h>
namespace ndnfd {

class SimGlobal;
class SimAppFace;
class Message;
class NdnfdSim;
class Name;

class L3Protocol : public ns3::ndn::L3Protocol {
 public:
  static ns3::Time kMinStartTime(void) { return ns3::MicroSeconds(16000000); }

  static ns3::TypeId GetTypeId(void);
  L3Protocol(void);
  virtual ~L3Protocol(void);
  SimGlobal* global(void) const { NS_ASSERT(this->global_ != nullptr); return this->global_; }

  // Init initializes this L3Protocol,
  // and aggregate this to node.
  void Init(ns3::Ptr<ns3::Node> node);

  // AddFace registers an AppFace, and returns face id.
  virtual uint32_t AddFace(const ns3::Ptr<ns3::ndn::Face>& face);
  // RemoveFace deletes a Face.
  virtual void RemoveFace(ns3::Ptr<ns3::ndn::Face> face);

  // GetFaceById returns the face with id.
  virtual ns3::Ptr<ns3::ndn::Face> GetFaceById(uint32_t id) const;
  
  // GetNFaces returns the number of faces.
  virtual uint32_t GetNFaces(void) const { return this->facelist_.size(); }
  // GetFace returns the face at index.
  // This method is inefficient. Use GetFaceById when possible.
  virtual ns3::Ptr<ns3::ndn::Face> GetFace(uint32_t index) const;
  
  // GetFaceByNetDevice is not supported because lower layer is provided by NDNFD core.
  virtual ns3::Ptr<ns3::ndn::Face> GetFaceByNetDevice(ns3::Ptr<ns3::NetDevice> netDevice) const { return nullptr; }

  // AppSend delivers a message to AppFace.
  void AppSend(SimAppFace* aface, const Message* msg);

 private:
  SimGlobal* global_;
  std::map<uint32_t,ns3::Ptr<ns3::ndn::Face>> facelist_;
  ns3::TracedCallback<ns3::Ptr<L3Protocol>, const Name*> trace_mcast_send_;
  ns3::TracedCallback<ns3::Ptr<L3Protocol>, const Name*> trace_mcast_recv_;
  ns3::TracedCallback<ns3::Ptr<L3Protocol>, const Name*> trace_unicast_send_;
  
  uint32_t nodeid(void) const;
  
  // AppReceive receives a message from AppFace, and pass it to NDNFD.
  void AppReceive(const ns3::Ptr<ns3::ndn::Face>& face, const ns3::Ptr<const ns3::Packet>& p);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_L3PROTOCOL_H_
