#ifndef CCND2_FACE_UDP_H_
#define CCND2_FACE_UDP_H_
#include "util/defs.h"
#include "face/face.h"
#include "util/pollmgr.h"
namespace ccnd2 {

class UdpFace : public Face {
  public:
    virtual void Send(Ptr<Message> message);

  private:
    DISALLOW_COPY_AND_ASSIGN(UdpFace);
};

class UdpFaceMaster : public FaceMaster, public IPollClient {
  public:
    UdpFaceMaster(void);
    
    void SetupUnicast(const NetworkAddress& local_addr);
    void SetupMulticast(const NetworkAddress& local_addr, const NetworkAddress& mcast_addr);

    virtual void PollCallback(int fd, short revents);

    virtual Ptr<Face> listener(void) { return this->listener_; }

    virtual Ptr<Face> multicast(void) { return this->multicast_; }
    
    virtual Ptr<Face> unicast(PeerAddress peer);
    
  private:
    int fd_unicast_;
    int fd_multicast_send_;
    int fd_multicast_recv_;
    
    Ptr<UdpFace> listener_;
    Ptr<UdpFace> multicast_;
    std::unordered_map<PeerAddress,Ptr<UdpFace>> unicast_;
    
    DISALLOW_COPY_AND_ASSIGN(UdpFaceMaster);
};

};//namespace ccnd2
#endif//CCND2_FACE_UDP_H
