#include "ether.h"
#include <net/ethernet.h>
#ifdef __linux__
#include <netinet/ether.h>
#endif
#include <sys/ioctl.h>
#include <net/if.h>
#ifndef SIOCGIFHWADDR
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif
#include "util/endian.h"
#include "core/scheduler.h"
#include "face/ndnlp.h"

namespace ndnfd {

PcapChannel::PcapChannel(const std::string& ifname, uint16_t ether_type, pcap_t* p, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) : DgramChannel(pcap_get_selectable_fd(p), local_addr, av, wp){
  this->ifname_ = ifname;
  this->p_ = p;
  this->ether_type_ = ether_type;
  this->local_ip_ = 0;
  this->ip_mcast_fd_ = -1;
}

void PcapChannel::Init(void) {
  DgramChannel::Init();
  
  int res;
  char filter_text[256];
  snprintf(filter_text, sizeof(filter_text), "ether proto 0x%" PRIx16 "", this->ether_type_);
  res = pcap_compile(this->p_, &this->filter_, filter_text, 1, 0);
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::Init pcap_compile %s", this->ifname_.c_str(), pcap_geterr(this->p_));
  }
  
  res = pcap_setfilter(this->p_, &this->filter_);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::Init pcap_setfilter %s", this->ifname_.c_str(), pcap_geterr(this->p_));
  }
  
#ifdef __FreeBSD__
  // on FreeBSD, poll() does not return when multicast frame arrives
  this->global()->scheduler()->Schedule(std::chrono::microseconds(1000), std::bind(&PcapChannel::ScheduledReceive, this));
#endif
}

void PcapChannel::CloseFd(void) {
  pcap_close(this->p_);
  pcap_freecode(&this->filter_);
  if (this->ip_mcast_fd_ != -1) close(this->ip_mcast_fd_);
}

void PcapChannel::SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  ether_header* hdr = reinterpret_cast<ether_header*>(pkt->Push(sizeof(ether_header)));
  memcpy(hdr->ether_dhost, reinterpret_cast<const ether_addr*>(&peer.who), sizeof(hdr->ether_dhost));
  memcpy(hdr->ether_shost, reinterpret_cast<const ether_addr*>(&this->local_addr()), sizeof(hdr->ether_shost));
  hdr->ether_type = htobe16(this->ether_type_);

  int res = pcap_inject(this->p_, pkt->data(), pkt->length());
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::SendTo pcap_inject %s", this->ifname_.c_str(), pcap_geterr(this->p_));
  }
}

std::chrono::microseconds PcapChannel::ScheduledReceive(void) {
  this->ReceiveFrom();
  return std::chrono::microseconds(10000);
}

void PcapChannel::ReceiveFrom(void) {
  int res = pcap_dispatch(this->p_, -1, &PcapChannel::DispatchHandler, reinterpret_cast<u_char*>(this));
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::ReceiveFrom pcap_dispatch %s", this->ifname_.c_str(), pcap_geterr(this->p_));
  }
}

void PcapChannel::DispatchHandler(u_char* user, const pcap_pkthdr* h, const u_char* bytes) {
  PcapChannel* self = reinterpret_cast<PcapChannel*>(user);
  if (h->caplen != h->len) {
    self->Log(kLLWarn, kLCFace, "PcapChannel(%s)::DispatchHandler h->caplen %" PRIu32 " != h->len %" PRIu32 "", self->ifname_.c_str(), h->caplen, h->len);
    return;
  }
  Ptr<EtherPkt> pkt = new EtherPkt(bytes, h->len);
  Ptr<McastEntry> entry = self->FindMcastEntry(pkt->local());
  if (entry != nullptr) {
    self->DeliverMcastPacket(entry, pkt->peer(), pkt);
  } else {
    self->DeliverPacket(pkt->peer(), pkt);
  }
}

PcapChannel::EtherPkt::EtherPkt(const uint8_t* pkt, size_t length) {
  this->pkt_ = pkt + sizeof(ether_header);
  this->length_ = length - sizeof(ether_header);
}

NetworkAddress PcapChannel::EtherPkt::peer(void) const {
  const ether_header* hdr = reinterpret_cast<const ether_header*>(this->pkt_ - sizeof(ether_header));
  NetworkAddress addr;
  addr.wholen = sizeof(hdr->ether_shost);
  memcpy(&addr.who, hdr->ether_shost, addr.wholen);
  return addr;
}

NetworkAddress PcapChannel::EtherPkt::local(void) const {
  const ether_header* hdr = reinterpret_cast<const ether_header*>(this->pkt_ - sizeof(ether_header));
  NetworkAddress addr;
  addr.wholen = sizeof(hdr->ether_shost);
  memcpy(&addr.who, hdr->ether_dhost, addr.wholen);
  return addr;
}

Ptr<DgramFace> PcapChannel::CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group) {
  if (!this->JoinIpMcast(group)) return nullptr;

  Ptr<DgramFace> face = this->New<DgramFace>(this, group);
  face->set_kind(FaceKind::kMulticast);
  this->Log(kLLInfo, kLCFace, "PcapChannel(%s)::CreateMcastFace id=%" PRI_FaceId " group=%s", this->ifname_.c_str(), face->id(), this->av()->ToString(group).c_str());
  return face;
}

bool PcapChannel::JoinIpMcast(const NetworkAddress& group) {
  if (this->local_ip_ == 0U) return false;
  const uint8_t* ea = reinterpret_cast<const uint8_t*>(&group.who);
  if (!(ea[0] == 0x01 && ea[1] == 0x00 && ea[2] == 0x5E && (ea[3] & 0x80) == 0x00)) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::JoinIpMcast(%s) out of IPv4 multicast Ethernet address range", this->ifname_.c_str(), this->av()->ToString(group).c_str());
    return false;
  }

  if (this->ip_mcast_fd_ == -1) {
    this->ip_mcast_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htobe16(9697);
    sa.sin_addr.s_addr = this->local_ip_;
    if (0 != bind(this->ip_mcast_fd_, reinterpret_cast<sockaddr*>(&sa), sizeof(sa))) {
      this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::JoinIpMcast(%s) bind(%" PRIx32 ") %s", this->ifname_.c_str(), this->av()->ToString(group).c_str(), be32toh(sa.sin_addr.s_addr), Logging::ErrorString().c_str());
      return false;
    }
  }
  ip_mreqn mreq;
  mreq.imr_multiaddr.s_addr = htobe32(0xE2800000U | (static_cast<uint32_t>(ea[3] & 0x7F) << 16) | (static_cast<uint32_t>(ea[4]) << 8) | static_cast<uint32_t>(ea[5]));
  mreq.imr_address.s_addr = this->local_ip_;
  mreq.imr_ifindex = 0;
  int res = setsockopt(this->ip_mcast_fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::JoinIpMcast(%s) IP_ADD_MEMBERSHIP(%" PRIx32 ") %s", this->ifname_.c_str(), this->av()->ToString(group).c_str(), be32toh(mreq.imr_multiaddr.s_addr), Logging::ErrorString().c_str());
    return false;
  }
  return true;
}

bool EtherAddressVerifier::Check(const NetworkAddress& addr) {
  return addr.wholen == sizeof(ether_addr);
}

AddressHashKey EtherAddressVerifier::GetHashKey(const NetworkAddress& addr) {
  return AddressHashKey(reinterpret_cast<const char*>(&addr.who), sizeof(ether_addr));
}

std::string EtherAddressVerifier::ToString(const NetworkAddress& addr) {
  const ether_addr* ea = reinterpret_cast<const ether_addr*>(&addr.who);
  return std::string(ether_ntoa(ea));
}

std::tuple<bool,NetworkAddress> EtherAddressVerifier::Parse(std::string s) {
  NetworkAddress addr;
  ether_addr* ea = ether_aton(s.c_str());
  if (ea == nullptr) {
    return std::forward_as_tuple(false, addr);
  }
  addr.wholen = sizeof(ether_addr);
  memcpy(&addr.who, ea, addr.wholen);
  return std::forward_as_tuple(true, addr);
}

EtherFaceFactory::EtherFaceFactory(void) {
  this->av_ = new EtherAddressVerifier();
}

std::vector<std::string> EtherFaceFactory::ListNICs(void) {
  std::vector<std::string> r;

  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t* alldevs;
  int res = pcap_findalldevs(&alldevs, errbuf);
  if (res != 0) return r;
  for (pcap_if_t* dev = alldevs; dev != nullptr; dev = dev->next) {
    if (strcmp("any", dev->name) == 0 || (dev->flags & PCAP_IF_LOOPBACK) != 0) continue;
    r.emplace_back(dev->name);
  }
  pcap_freealldevs(alldevs);
  return r;
}
  
Ptr<DgramChannel> EtherFaceFactory::Channel(const std::string& ifname, uint16_t ether_type) {
  bool ok; int mtu; NetworkAddress local_addr; uint32_t local_ip;
  std::tie(ok, mtu) = this->GetIfMtu(ifname);
  if (!ok) return nullptr;
  std::tie(ok, local_addr, local_ip) = this->GetIfAddr(ifname);
  if (!ok) return nullptr;

  Ptr<NdnlpWireProtocol> ndnlp = this->New<NdnlpWireProtocol>(mtu);

  pcap_t* p = this->PcapOpen(ifname);
  if (p == nullptr) return nullptr;
  Ptr<PcapChannel> channel = this->New<PcapChannel>(ifname, ether_type, p, local_addr, this->av_, ndnlp);
  channel->set_local_ip(local_ip);
  return channel;
}

std::tuple<bool,int> EtherFaceFactory::GetIfMtu(const std::string& ifname) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  ifreq ifr;
  strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
  int res = ioctl(sock, SIOCGIFMTU, &ifr);
  close(sock);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::GetIfMtu(%s) %s", ifname.c_str(), Logging::ErrorString().c_str());
  }
  return std::forward_as_tuple(res == 0, ifr.ifr_mtu);
}

std::tuple<bool,NetworkAddress,uint32_t> EtherFaceFactory::GetIfAddr(const std::string& ifname) {
  bool ok = false; int res;
  NetworkAddress local_addr;
  uint32_t local_ip = 0;

#ifdef SIOCGIFHWADDR
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  ifreq ifr;
  strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));

  res = ioctl(sock, SIOCGIFHWADDR, &ifr);
  if (res == 0) {
    local_addr.wholen = sizeof(ether_addr);
    memcpy(&local_addr.who, ifr.ifr_hwaddr.sa_data, local_addr.wholen);
  } else {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::GetIfAddr(%s) SIOCGIFHWADDR %s", ifname.c_str(), Logging::ErrorString().c_str());
  }

  res = ioctl(sock, SIOCGIFADDR, &ifr);
  if (res == 0) {
    sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr);
    local_ip = sa->sin_addr.s_addr;
  } else {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::GetIfAddr(%s) SIOCGIFADDR %s", ifname.c_str(), Logging::ErrorString().c_str());
  }
  close(sock);
#else
  ifaddrs* ifa_list;
  res = getifaddrs(&ifa_list);
  if (res == 0) {  
    for (ifaddrs* ifa = ifa_list; ifa != nullptr; ifa = ifa->ifa_next) {
      if (ifname.compare(ifa->ifa_name) != 0) continue;
      if (ifa->ifa_addr->sa_family == AF_LINK) {
        sockaddr_dl* sa = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
        local_addr.wholen = sa->sdl_alen;
        memcpy(&local_addr.who, LLADDR(sa), local_addr.wholen);
      } else if (ifa->ifa_addr->sa_family == AF_INET) {
        sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        local_ip = sa->sin_addr.s_addr;
      }
    }
    freeifaddrs(ifa_list);
  } else {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::GetIfAddr(%s) getifaddrs %s", ifname.c_str(), Logging::ErrorString().c_str());
  }
#endif

  ok = this->av_->Check(local_addr) && local_ip != 0U;
  if (!ok) {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::GetIfAddr(%s) fail", ifname.c_str());
  }
  return std::forward_as_tuple(ok, local_addr, local_ip);
}

pcap_t* EtherFaceFactory::PcapOpen(const std::string& ifname) {
  char errbuf[PCAP_ERRBUF_SIZE];
  
  const int promisc = 0;
  errbuf[0] = '\0';
  pcap_t* p = pcap_open_live(ifname.c_str(), 65535, promisc, 0, errbuf);
  if (p == nullptr) {
    this->Log(kLLError, kLCFace, "EtherFaceFactory::PcapOpen(%s) pcap_open_live error %s", ifname.c_str(), errbuf);
    return nullptr;
  } else if (errbuf[0] != '\0') {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::PcapOpen(%s) pcap_open_live %s", ifname.c_str(), errbuf);
  } else {
    this->Log(kLLInfo, kLCFace, "EtherFaceFactory::PcapOpen(%s) pcap_open_live OK", ifname.c_str());
  }
  
  int res;
  errbuf[0] = '\0';
  res = pcap_setnonblock(p, 1, errbuf);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "EtherFaceFactory::PcapOpen(%s) pcap_set_nonblock %s", ifname.c_str(), errbuf);
  }
  
  return p;
}

};//namespace ndnfd
