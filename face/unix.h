#ifndef NDNFD_FACE_UNIX_H_
#define NDNFD_FACE_UNIX_H_
#include "face/stream.h"
namespace ndnfd {

class UnixFaceFactory : Element {
  public:
    Ptr<Face> MakeListener(const std::string& local_socket);
  private:
    DISALLOW_COPY_AND_ASSIGN(UnixFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UNIX_H
