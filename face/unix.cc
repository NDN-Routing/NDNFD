#include "unix.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
namespace ndnfd {

bool UnixAddressVerifier::Check(const NetworkAddress& addr) const {
  return addr.family() == AF_UNIX;
}

std::string UnixAddressVerifier::ToString(const NetworkAddress& addr) const {
  const sockaddr_un* sa = reinterpret_cast<const sockaddr_un*>(&addr.who);
  return std::string(sa->sun_path);
}

void UnixListener::Close(void) {
  StreamListener::Close();
  unlink(this->local_socket_.c_str());
}

UnixFaceFactory::UnixFaceFactory(Ptr<WireProtocol> wp) : FaceFactory(wp) {
  this->av_ = new UnixAddressVerifier();
}

Ptr<StreamListener> UnixFaceFactory::Listen(const std::string& local_socket) {
  NetworkAddress local_addr; local_addr.wholen = sizeof(sockaddr_un);
  sockaddr_un* sa = reinterpret_cast<sockaddr_un*>(&local_addr.who);
  if (local_socket.size() >= sizeof(sa->sun_path)) {
    this->Log(kLLWarn, kLCFace, "UnixFaceFactory::Listen path too long");
    return nullptr;
  }
  sa->sun_family = AF_UNIX;
  strncpy(sa->sun_path, local_socket.c_str(), sizeof(sa->sun_path));
  
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    this->Log(kLLWarn, kLCFace, "UnixFaceFactory::Listen socket(): %s", Logging::ErrorString().c_str());
    return nullptr;
  }
  int res;
  res = unlink(sa->sun_path);
  if (res == 0) {
    this->Log(kLLInfo, kLCFace, "UnixFaceFactory::Listen unlink(%s)", sa->sun_path);
  }
  
  int umask_save = umask(0111);
  res = bind(fd, reinterpret_cast<const sockaddr*>(&local_addr.who), local_addr.wholen);
  umask(umask_save);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "UnixFaceFactory::Listen bind(): %s", Logging::ErrorString().c_str());
    close(fd);
    return nullptr;
  }
  res = listen(fd, 30);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "UnixFaceFactory::Listen listen(): %s", Logging::ErrorString().c_str());
    close(fd);
    return nullptr;
  }
  Ptr<UnixListener> face = this->New<UnixListener>(fd, this->av_, this->wp(), local_socket);
  face->set_accepted_kind(FaceKind::kApp);
  return face;
}

};//namespace ndnfd
