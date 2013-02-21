#include "ip.h"
#include <fcntl.h>
#include <arpa/inet.h>
#include "util/socket_helper.h"
namespace ndnfd {

bool IpAddressVerifier::Check(const NetworkAddress& addr) {
  return (addr.wholen == sizeof(sockaddr_in) && addr.family() == AF_INET)
         || (addr.wholen == sizeof(sockaddr_in6) && addr.family() == AF_INET6);
}

NetworkAddress IpAddressVerifier::Normalize(const NetworkAddress& addr) {
  NetworkAddress n; n.wholen = addr.wholen;
  switch (addr.family()) {
    case AF_INET: {
      const sockaddr_in* src = reinterpret_cast<const sockaddr_in*>(&addr.who);
      sockaddr_in* dst = reinterpret_cast<sockaddr_in*>(&n.who);
      dst->sin_family = AF_INET;
      dst->sin_port = src->sin_port;
      dst->sin_addr.s_addr = src->sin_addr.s_addr;
      } break;
    case AF_INET6: {
      const sockaddr_in6* src = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      sockaddr_in6* dst = reinterpret_cast<sockaddr_in6*>(&n.who);
      dst->sin6_family = AF_INET6;
      dst->sin6_port = src->sin6_port;
      memcpy(dst->sin6_addr.s6_addr, src->sin6_addr.s6_addr, sizeof(dst->sin6_addr.s6_addr));
      } break;
    default: assert(false); break;
  }
  return n;
}

bool IpAddressVerifier::IsLocal(const NetworkAddress& addr) {
  switch (addr.family()) {
    case AF_INET: {
      const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&addr.who);
      const uint8_t* rawaddr = reinterpret_cast<const uint8_t*>(&sa->sin_addr.s_addr);
      if (rawaddr[0] == 127) return true;
    } break;
    case AF_INET6: {
      const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      #ifdef IN6_IS_ADDR_LOOPBACK
      if (IN6_IS_ADDR_LOOPBACK(&sa->sin6_addr)) return true;
      #endif
  } break;
    default: assert(false); break;
  }
  return false;
}

bool IpAddressVerifier::AreSameHost(const NetworkAddress& a, const NetworkAddress& b) {
  if (a.family() != b.family()) return false;
  switch (a.family()) {
    case AF_INET: {
      const sockaddr_in* sa_a = reinterpret_cast<const sockaddr_in*>(&a.who);
      const sockaddr_in* sa_b = reinterpret_cast<const sockaddr_in*>(&b.who);
      if (sa_a->sin_addr.s_addr == sa_b->sin_addr.s_addr) return true;
    } break;
    case AF_INET6: {
      const sockaddr_in6* sa_a = reinterpret_cast<const sockaddr_in6*>(&a.who);
      const sockaddr_in6* sa_b = reinterpret_cast<const sockaddr_in6*>(&b.who);
      if (0 == memcmp(sa_a->sin6_addr.s6_addr, sa_b->sin6_addr.s6_addr, sizeof(sa_a->sin6_addr.s6_addr))) return true;
  } break;
    default: assert(false); break;
  }
  return false;
}

std::string IpAddressVerifier::ToString(const NetworkAddress& addr) {
  char buf[INET6_ADDRSTRLEN];
  std::string r;
  in_port_t port = 0;
  switch (addr.family()) {
    case AF_INET: {
      const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&addr.who);
      inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
      r.append(buf);
      port = sa->sin_port;
    } break;
    case AF_INET6: {
      const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      inet_ntop(AF_INET6, &sa->sin6_addr, buf, sizeof(buf));
      r.push_back('[');
      r.append(buf);
      r.push_back(']');
      port = sa->sin6_port;
    } break;
    default: assert(false); break;
  }
  sprintf(buf, ":%d", be16toh(port));
  r.append(buf);
  return r;
}

std::tuple<bool,NetworkAddress> IpAddressVerifier::Parse(std::string s) {
  NetworkAddress addr;
  size_t port_delim = s.find_last_of(":");
  in_port_t port = 0;
  if (port_delim != std::string::npos) {
    port = htobe16(static_cast<uint16_t>(atoi(s.c_str()+port_delim+1)));
    s.erase(port_delim);
  }
  if (s[0] == '[' && s[s.size()-1] == ']') {
    s.erase(0, 1);
    s.erase(s.size()-1);
    // don't try IPv4 if host is wrapped by [ ]
  } else {
    sockaddr_in* sa4 = reinterpret_cast<sockaddr_in*>(&addr.who);
    if (1 == inet_pton(AF_INET, s.c_str(), &sa4->sin_addr)) {
      sa4->sin_family = AF_INET;
      sa4->sin_port = port;
      addr.wholen = sizeof(*sa4);
      return std::forward_as_tuple(true, addr);
    }
  }
  
  sockaddr_in6* sa6 = reinterpret_cast<sockaddr_in6*>(&addr.who);
  if (1 == inet_pton(AF_INET6, s.c_str(), &sa6->sin6_addr)) {
    sa6->sin6_family = AF_INET6;
    sa6->sin6_port = port;
    addr.wholen = sizeof(*sa6);
    return std::forward_as_tuple(true, addr);
  }
  return std::forward_as_tuple(false, addr);
}

UdpFaceFactory::UdpFaceFactory(Ptr<WireProtocol> wp) : FaceFactory(wp) {
  this->av_ = new IpAddressVerifier();
}

Ptr<DgramChannel> UdpFaceFactory::Channel(const NetworkAddress& local_addr) {
  int fd = Socket_CreateForListen(local_addr.family(), SOCK_DGRAM);
  if (fd < 0) {
    this->Log(kLLWarn, kLCFace, "UdpFaceFactory::Channel socket(): %s", Logging::ErrorString().c_str());
    return nullptr;
  }
  
  int res;
  res = bind(fd, reinterpret_cast<const sockaddr*>(&local_addr.who), local_addr.wholen);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "UdpFaceFactory::Channel bind(): %s", Logging::ErrorString().c_str());
    close(fd);
    return nullptr;
  }
  Ptr<DgramChannel> channel = this->New<DgramChannel>(fd, local_addr, this->av_, this->wp());
  return channel;
}

TcpFaceFactory::TcpFaceFactory(Ptr<WireProtocol> wp) : FaceFactory(wp) {
  this->av_ = new IpAddressVerifier();
}

Ptr<StreamListener> TcpFaceFactory::Listen(const NetworkAddress& local_addr) {
  int fd = Socket_CreateForListen(local_addr.family(), SOCK_STREAM);
  if (fd < 0) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::Listen socket(): %s", Logging::ErrorString().c_str());
    return nullptr;
  }
  
  int res;
  res = bind(fd, reinterpret_cast<const sockaddr*>(&local_addr.who), local_addr.wholen);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::Listen bind(): %s", Logging::ErrorString().c_str());
    close(fd);
    return nullptr;
  }
  res = listen(fd, 30);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::Listen listen(): %s", Logging::ErrorString().c_str());
    close(fd);
    return nullptr;
  }
  Ptr<StreamListener> face = this->New<StreamListener>(fd, this->av_, this->wp());
  return face;
}

Ptr<StreamFace> TcpFaceFactory::Connect(const NetworkAddress& remote_addr) {
  int fd = socket(remote_addr.family(), SOCK_STREAM, 0);
  if (fd < 0) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::Connect socket(): %s", Logging::ErrorString().c_str());
    return nullptr;
  }

  int res = fcntl(fd, F_SETFL, O_NONBLOCK);
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::Connect fcntl(O_NONBLOCK) %s", Logging::ErrorString().c_str());
  }
  res = connect(fd, reinterpret_cast<const sockaddr*>(&remote_addr.who), remote_addr.wholen);
  if (res == -1 && errno != EINPROGRESS) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::Connect connect(%s) %s", this->av_->ToString(remote_addr).c_str(), Logging::ErrorString().c_str());
  }
  Ptr<StreamFace> face = this->New<StreamFace>(fd, true, remote_addr, this->wp());
  return face;
}

};//namespace ndnfd
