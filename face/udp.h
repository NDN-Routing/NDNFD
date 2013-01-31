#ifndef NDNFD_FACE_UDP_H_
#define NDNFD_FACE_UDP_H_
#include "face/dgram.h"
namespace ndnfd {

class UdpFaceFactory : Element {
  public:
    Ptr<Face> MakeMasterFace(const NetworkAddress& local_addr);
  private:
    DISALLOW_COPY_AND_ASSIGN(UdpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UDP_H
