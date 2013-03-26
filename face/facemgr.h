#ifndef NDNFD_FACE_FACEMGR_H_
#define NDNFD_FACE_FACEMGR_H_
extern "C" {
#include <ccn/face_mgmt.h>
}
#include "face/face.h"
#include "face/ccnd_interface.h"
#include "core/internal_client_handler.h"
namespace ndnfd {

class CcndFaceInterface;
class UnixFaceFactory;
class TcpFaceFactory;
class UdpFaceFactory;
class EtherFaceFactory;
class StreamListener;
class DgramChannel;
class DgramFace;

// A FaceMgr manages all Faces of a router.
class FaceMgr : public Element {
 public:
  // A FaceMgmtProtoAct represents an action in CCNx Face Management Protocol.
  enum class FaceMgmtProtoAct {
    kNone        = 0,
    kNewFace     = 1,
    kDestroyFace = 2
  };

  FaceMgr(void);
  virtual void Init(void);
  virtual ~FaceMgr(void);
  Ptr<CcndFaceInterface> ccnd_face_interface(void) const { return this->ccnd_face_interface_; }
  
  // GetFace finds a Face by FaceId.
  Ptr<Face> GetFace(FaceId id);
  // AddFace assigns FaceId to a new Face, and puts it in the table.
  // This is called by Face::Init.
  void AddFace(Ptr<Face> face);
  // RemoveFace removes Face from the table.
  // This is called by a closing Face after send queue is empty.
  // This can also be called to immediately close a Face.
  void RemoveFace(Ptr<Face> face);
  // NotifyStatusChange is called by Face when status is changed.
  void NotifyStatusChange(Ptr<Face> face);

  // StartDefaultListeners creates default listeners.
  virtual void StartDefaultListeners(void);

  StreamListener* unix_listener(void) const { return this->unix_listener_; }
  TcpFaceFactory* tcp_factory(void) const { return this->tcp_factory_; }
  StreamListener* tcp_listener(void) const { return this->tcp_listener_; }
  DgramChannel* udp_channel(void) const { return this->udp_channel_; }
  DgramFace* udp_mcast_face(void) const { return this->udp_mcast_face_; }
  DgramChannel* udp_ndnlp_channel(void) const { return this->udp_ndnlp_channel_; }
  const std::vector<std::tuple<std::string,DgramChannel*,DgramFace*>>& ether_channels(void) const { return this->ether_channels_; }
  DgramChannel* ether_channel(std::string ifname) const;

  // MakeUnicastFace finds or creates a unicast Face
  // from a message received on a multicast Face.
  Ptr<Face> MakeUnicastFace(Ptr<Face> mcast_face, const NetworkAddress& peer);
  
  // FaceMgmtReq answers a face management request.
  std::tuple<InternalClientHandler::ResponseKind,Ptr<Buffer>> FaceMgmtReq(FaceMgmtProtoAct act, FaceId inface, const uint8_t* msg, size_t size);

 protected:
  void set_unix_listener(Ptr<StreamListener> value);
  void set_tcp_factory(Ptr<TcpFaceFactory> value);
  void set_tcp_listener(Ptr<StreamListener> value);
  void set_udp_channel(Ptr<DgramChannel> value);
  void set_udp_mcast_face(Ptr<DgramFace> value);
  void set_udp_ndnlp_channel(Ptr<DgramChannel> value);
  void add_ether_channel(const std::string& ifname, Ptr<DgramChannel> channel, Ptr<DgramFace> mcast_face);

 private:
  FaceId next_id_;
  std::map<FaceId,Ptr<Face>> table_;
  Ptr<CcndFaceInterface> ccnd_face_interface_;
  
  StreamListener* unix_listener_;
  TcpFaceFactory* tcp_factory_;
  StreamListener* tcp_listener_;
  DgramChannel* udp_channel_;
  DgramFace* udp_mcast_face_;
  DgramChannel* udp_ndnlp_channel_;
  std::vector<std::tuple<std::string,DgramChannel*,DgramFace*>> ether_channels_;//Ethernet ifname,channel,mcast_face

  // FaceMgmtNewFace executes a face management protocol newface action.
  // Caller should ensure req is trusted (eg. coming from a local face).
  // On success, it returns true and updates face_inst for the response.
  // On failure, it reutrns false and an error message.
  std::tuple<bool,std::string> FaceMgmtNewFace(ccn_face_instance* face_inst);

  // FaceMgmtDestroyFace executes a face management protocol destroyface action.
  // Caller should ensure req is trusted (eg. coming from a local face).
  // On success, it returns true and updates face_inst for the response.
  // On failure, it reutrns false and an error message.
  std::tuple<bool,std::string> FaceMgmtDestroyFace(ccn_face_instance* face_inst);
  
  DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEMGR_H
