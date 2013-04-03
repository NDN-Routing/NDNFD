#include "ip.h"
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cstdio>
#include <ifaddrs.h>
#include <ccn/sockcreate.h>
#include "util/endian.h"
#include "util/socket_helper.h"
namespace ndnfd {

bool IpAddressVerifier::Check(const NetworkAddress& addr) const {
  return (addr.wholen == sizeof(sockaddr_in) && addr.family() == AF_INET)
         || (addr.wholen == sizeof(sockaddr_in6) && addr.family() == AF_INET6);
}

AddressHashKey IpAddressVerifier::GetHashKey(const NetworkAddress& addr) const {
  switch (addr.family()) {
    case AF_INET: {
      const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&addr.who);
      AddressHashKey h(reinterpret_cast<const char*>(&sa->sin_port), sizeof(sa->sin_port));
      h.append(reinterpret_cast<const char*>(&sa->sin_addr.s_addr), sizeof(sa->sin_addr.s_addr));
      return h;
    } break;
    case AF_INET6: {
      const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      AddressHashKey h(reinterpret_cast<const char*>(&sa->sin6_port), sizeof(sa->sin6_port));
      h.append(reinterpret_cast<const char*>(&sa->sin6_addr.s6_addr), sizeof(sa->sin6_addr.s6_addr));
      return h;
    } break;
    default: assert(false); break;
  }
}

bool IpAddressVerifier::IsLocal(const NetworkAddress& addr) const {
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

bool IpAddressVerifier::IsMcast(const NetworkAddress& addr) const {
  switch (reinterpret_cast<const sockaddr*>(&addr.who)->sa_family) {
    case AF_INET: {
      const sockaddr_in *sa = reinterpret_cast<const sockaddr_in*>(&addr.who);
      return (be32toh(sa->sin_addr.s_addr) & 0xF0000000) == 0xE0000000;
    }
    case AF_INET6: {
      const sockaddr_in6 *sa6 = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      return sa6->sin6_addr.s6_addr[0] == 0xFF;
    }
  }
  return false;
}

bool IpAddressVerifier::AreSameHost(const NetworkAddress& a, const NetworkAddress& b) const {
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

std::string IpAddressVerifier::IpToString(const NetworkAddress& addr) const {
  char buf[INET6_ADDRSTRLEN];
  switch (addr.family()) {
    case AF_INET: {
      const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&addr.who);
      inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
    } break;
    case AF_INET6: {
      const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      inet_ntop(AF_INET6, &sa->sin6_addr, buf, sizeof(buf));
    } break;
    default: assert(false); break;
  }
  return std::string(buf);
}

uint16_t IpAddressVerifier::GetPort(const NetworkAddress& addr) const {
  in_port_t port = 0;
  switch (addr.family()) {
    case AF_INET: {
      const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(&addr.who);
      port = sa->sin_port;
    } break;
    case AF_INET6: {
      const sockaddr_in6* sa = reinterpret_cast<const sockaddr_in6*>(&addr.who);
      port = sa->sin6_port;
    } break;
    default: assert(false); break;
  }
  return be16toh(port);
}

std::string IpAddressVerifier::ToString(const NetworkAddress& addr) const {
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

std::tuple<bool,int> UdpFaceFactory::MakeBoundSocket(const NetworkAddress& local_addr) {
  int fd = Socket_CreateForListen(local_addr.family(), SOCK_DGRAM);
  if (fd < 0) {
    this->Log(kLLWarn, kLCFace, "UdpFaceFactory::MakeBoundSocket socket(): %s", Logging::ErrorString().c_str());
    return std::forward_as_tuple(false, -1);
  }
  
  int res;
  res = bind(fd, reinterpret_cast<const sockaddr*>(&local_addr.who), local_addr.wholen);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "UdpFaceFactory::MakeBoundSocket bind(): %s", Logging::ErrorString().c_str());
    close(fd);
    return std::forward_as_tuple(false, -1);
  }

  return std::forward_as_tuple(true, fd);
}

Ptr<DgramChannel> UdpFaceFactory::Channel(const NetworkAddress& local_addr) {
  bool ok; int fd;
  std::tie(ok, fd) = this->MakeBoundSocket(local_addr);
  if (!ok) return nullptr;
  
  Ptr<DgramChannel> channel = this->New<DgramChannel>(fd, local_addr, this->av_, this->wp());
  return channel;
}

std::vector<NetworkAddress> UdpFaceFactory::ListLocalAddresses(void) const {
  std::vector<NetworkAddress> ips;
  std::unordered_set<std::string> ifnames;

  ifaddrs* ifa_list;
  int res = getifaddrs(&ifa_list);
  if (res != 0) return ips;
  
  for (ifaddrs* ifa = ifa_list; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family != AF_INET) continue;

    auto ifname_insert = ifnames.insert(ifa->ifa_name);
    if (!ifname_insert.second) continue;//another IP address of same NIC is already included
    
    ips.emplace_back();
    ips.back().wholen = sizeof(sockaddr_in);
    memcpy(&ips.back().who, ifa->ifa_addr, ips.back().wholen);
  }
  freeifaddrs(ifa_list);
  
  return ips;
}

Ptr<DgramFace> UdpFaceFactory::McastFace(const NetworkAddress& local_addr, const NetworkAddress& group_addr, uint8_t ttl) {
  std::string local_address = this->av_->IpToString(local_addr);
  uint16_t local_port = this->av_->GetPort(local_addr);
  std::string group_address = this->av_->IpToString(group_addr);
  uint16_t group_port = this->av_->GetPort(group_addr);
  assert(local_port == group_port);
  char port_buf[6]; snprintf(port_buf, sizeof(port_buf), "%" PRIu16 "", group_port);

  ccn_sockdescr descr;
  descr.ipproto = IPPROTO_UDP;
  descr.address = group_address.c_str();
  descr.port = port_buf;
  descr.source_address = local_address.c_str();
  descr.mcast_ttl = ttl;
  ccn_sockets socks;
  
  char logbuf[200];
  int res = ccn_setup_socket(&descr, reinterpret_cast<void (*)(void*, const char*, ...)>(&sprintf), logbuf, nullptr, nullptr, &socks);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "UdpFaceFactory::McastFace(%s,%s) %s", this->av_->ToString(local_addr).c_str(), this->av_->ToString(group_addr).c_str(), logbuf);
    return nullptr;
  }
  Ptr<DgramChannel> channel = this->New<UdpSingleMcastChannel>(socks.recving, socks.sending, local_addr, group_addr, this->av_, this->wp());
  Ptr<DgramFace> face = channel->GetMcastFace(group_addr);
  this->Log(kLLInfo, kLCFace, "UdpFaceFactory::McastFace(%s,%s) face=%" PRI_FaceId "", this->av_->ToString(local_addr).c_str(), this->av_->ToString(group_addr).c_str(), face->id());
  return face;
}

UdpSingleMcastChannel::UdpSingleMcastChannel(int recv_fd, int send_fd, const NetworkAddress& local_addr, const NetworkAddress& group_addr, Ptr<const AddressVerifier> av, Ptr<const WireProtocol> wp) : DgramChannel(recv_fd, local_addr, av, wp) {
  this->send_fd_ = send_fd;
  this->group_addr_ = group_addr;
}

Ptr<DgramFace> UdpSingleMcastChannel::CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group) {
  assert(this->group_addr_ == group);
  return this->New<UdpSingleMcastFace>(this, group);
}

void UdpSingleMcastChannel::DeliverPacket(const NetworkAddress& peer, Ptr<BufferView> pkt) {
  if (this->group_entry_ == nullptr) {
    this->group_entry_ = this->FindMcastEntry(this->group_addr_);
  }
  this->DeliverMcastPacket(this->group_entry_, peer, pkt);
}

void UdpSingleMcastChannel::SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  sendto(this->send_fd_, pkt->data(), pkt->length(), 0, reinterpret_cast<const sockaddr*>(&peer.who), peer.wholen);
}

TcpFaceFactory::TcpFaceFactory(Ptr<WireProtocol> wp) : FaceFactory(wp) {
  this->av_ = new IpAddressVerifier();
}

void TcpFaceFactory::Init(void) {
  this->fat_ = this->New<FaceAddressTable>(this->av_);
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

Ptr<StreamFace> TcpFaceFactory::FindFace(const NetworkAddress& remote_addr) const {
  FaceId faceid = this->fat_->Find(remote_addr);
  if (faceid == FaceId_none) return nullptr;
  Ptr<Face> face = this->global()->facemgr()->GetFace(faceid);
  return static_cast<StreamFace*>(PeekPointer(face));
}

Ptr<StreamFace> TcpFaceFactory::Connect(const NetworkAddress& remote_addr) {
  Ptr<StreamFace> face = this->FindFace(remote_addr);
  bool existing = true;
  if (face == nullptr) {
    existing = false;
    face = this->DoConnect(remote_addr);
  }
  this->Log(kLLInfo, kLCFace, "TcpFaceFactory::Connect(%s) %s face %" PRI_FaceId "", this->av_->ToString(remote_addr).c_str(), existing?"existing":"new", face==nullptr?FaceId_none:face->id());
  return face;
}

Ptr<StreamFace> TcpFaceFactory::DoConnect(const NetworkAddress& remote_addr) {
  int fd = socket(remote_addr.family(), SOCK_STREAM, 0);
  if (fd < 0) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::DoConnect socket(): %s", Logging::ErrorString().c_str());
    return nullptr;
  }

  int res = fcntl(fd, F_SETFL, O_NONBLOCK);
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::DoConnect fcntl(O_NONBLOCK) %s", Logging::ErrorString().c_str());
  }
  res = connect(fd, reinterpret_cast<const sockaddr*>(&remote_addr.who), remote_addr.wholen);
  if (res == -1 && errno != EINPROGRESS) {
    this->Log(kLLWarn, kLCFace, "TcpFaceFactory::DoConnect connect(%s) %s", this->av_->ToString(remote_addr).c_str(), Logging::ErrorString().c_str());
  }
  Ptr<StreamFace> face = this->New<StreamFace>(fd, true, remote_addr, this->wp());
  this->fat_->Add(remote_addr, face->id());
  return face;
}

};//namespace ndnfd
