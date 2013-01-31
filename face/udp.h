#ifndef NDNFD_FACE_UDP_H_
#define NDNFD_FACE_UDP_H_
#include "face/face.h"
namespace ndnfd {


class UdpMasterFace;

//communicate with one remote peer using UDP
class UdpFace : public Face {
  public:
    virtual void Send(Ptr<Message> message);

  private:
    Ptr<UdpMasterFace> master_;
    Ptr<Decoder> decoder_;
    DISALLOW_COPY_AND_ASSIGN(UdpFace);
};

//communicate with many remote peers at the same local socket using UDP
class UdpMasterFace : public UdpFace {
  public:
    UdpMasterFace(const NetworkAddress& local_addr);

    virtual bool CanSend(void) const { return false; }
    virtual void Send(Ptr<Message> message) { assert(false); }
    
    //create a unicast face sharing the same local socket
    Ptr<UdpFace> MakeUnicast(const NetworkAddress& remote_addr);

  private:
    Ptr<Channel> channel_;
    std::unordered_map<NetworkAddress,Ptr<UdpFace>> unicast_faces_;
    DISALLOW_COPY_AND_ASSIGN(UdpMasterFace);
};

//communicate on one UDP multicast group
class UdpMulticastFace : public UdpFace {
  public:
    UdpMulticastFace(const NetworkAddress& local_addr, const NetworkAddress& mcast_group);

    virtual void Send(Ptr<Message> message);
    
  private:
    Ptr<Channel> channel_;
    DISALLOW_COPY_AND_ASSIGN(UdpMasterFace);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UDP_H
