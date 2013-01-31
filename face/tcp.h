#ifndef NDNFD_FACE_TCP_H_
#define NDNFD_FACE_TCP_H_
#include "face/stream.h"
namespace ndnfd {

class TcpFaceFactory : public FaceFactory {
  public:
    bool CheckAddress(const NetworkAddress& addr);
    
    void NormalizeAddress(NetworkAddress& addr);

    Ptr<Face> MakeListener(const NetworkAddress& local_addr);

    Ptr<Face> MakeConnection(const NetworkAddress& remote_addr);

  private:
    DISALLOW_COPY_AND_ASSIGN(TcpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_TCP_H
