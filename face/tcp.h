#ifndef NDNFD_FACE_TCP_H_
#define NDNFD_FACE_TCP_H_
#include "face/stream.h"
namespace ndnfd {

class TcpFaceMaster : public FaceMaster {
  public:
    TcpFaceMaster(const NetworkAddress& local_addr);

    virtual Ptr<Face> listener(void) { return this->listener_; }

    virtual Ptr<Face> multicast(void) { return NULL; }
    
    virtual Ptr<Face> unicast(NetworkAddress peer);
    
    void ListenerAccept(int fd, NetworkAddress peer);
  private:
    Ptr<StreamListener> listener_;
    std::unordered_map<NetworkAddress,Ptr<StreamFace>> unicast_;
    
    DISALLOW_COPY_AND_ASSIGN(TcpFaceMaster);
};

};//namespace ndnfd
#endif//NDNFD_FACE_TCP_H
