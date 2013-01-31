#ifndef NDNFD_FACE_UDP_H_
#define NDNFD_FACE_UDP_H_
#include "face/dgram.h"
namespace ndnfd {

class UdpFaceFactory : public FaceFactory {
  public:
    bool CheckAddress(const NetworkAddress& addr);
    
    void NormalizeAddress(NetworkAddress& addr);

    Ptr<UdpMasterFace> MakeMasterFace(const NetworkAddress& local_addr);
  private:
    DISALLOW_COPY_AND_ASSIGN(UdpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UDP_H
