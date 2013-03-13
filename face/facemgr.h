#ifndef NDNFD_FACE_FACEMGR_H_
#define NDNFD_FACE_FACEMGR_H_
extern "C" {
#include <ccn/face_mgmt.h>
}
#include "face/face.h"
#include "face/ccnd_interface.h"
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
  void StartDefaultListeners(void);

  StreamListener* unix_listener(void) const { return this->unix_listener_; }
  TcpFaceFactory* tcp_factory(void) const { return this->tcp_factory_; }
  StreamListener* tcp_listener(void) const { return this->tcp_listener_; }
  DgramChannel* udp_channel(void) const { return this->udp_channel_; }
  DgramChannel* udp_ndnlp_channel(void) const { return this->udp_ndnlp_channel_; }
  std::vector<std::tuple<std::string,DgramChannel*,DgramFace*>>& ether_channels(void) { return this->ether_channels_; }

  // MakeUnicastFace finds or creates a unicast Face
  // from a message received on a multicast Face.
  Ptr<Face> MakeUnicastFace(Ptr<Face> mcast_face, const NetworkAddress& peer);
  // FaceMgmtRequest answers a face management request.
  // Caller should ensure req is trusted (eg. coming from a local face).
  // It returns whether success, and a reply ContentObject / ContentNack.
  std::tuple<bool,Ptr<Message>> FaceMgmtRequest(const ccn_face_instance* req);
  
 private:
  FaceId next_id_;
  std::map<FaceId,Ptr<Face>> table_;
  Ptr<CcndFaceInterface> ccnd_face_interface_;
  
  StreamListener* unix_listener_;
  TcpFaceFactory* tcp_factory_;
  StreamListener* tcp_listener_;
  DgramChannel* udp_channel_;
  DgramChannel* udp_ndnlp_channel_;
  std::vector<std::tuple<std::string,DgramChannel*,DgramFace*>> ether_channels_;//Ethernet ifname,channel,mcast_face

  void set_unix_listener(Ptr<StreamListener> value);
  void set_tcp_factory(Ptr<TcpFaceFactory> value);
  void set_tcp_listener(Ptr<StreamListener> value);
  void set_udp_channel(Ptr<DgramChannel> value);
  void set_udp_ndnlp_channel(Ptr<DgramChannel> value);

  DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEMGR_H
