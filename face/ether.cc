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
#include "face/ndnlp.h"

namespace ndnfd {

PcapChannel::PcapChannel(const std::string& ifname, uint16_t ether_type, pcap_t* p, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) : DgramChannel(pcap_get_selectable_fd(p), local_addr, av, wp){
  this->ifname_ = ifname;
  this->p_ = p;
  this->ether_type_ = ether_type;
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
}

void PcapChannel::CloseFd(void) {
  if (this->p_ == nullptr) return;
  pcap_close(this->p_);
  pcap_freecode(&this->filter_);
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

void PcapChannel::ReceiveFrom(void) {
  int res = pcap_dispatch(this->p_, -1, &PcapChannel::DispatchHandler, reinterpret_cast<u_char*>(this));
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::ReceiveFrom pcap_dispatch %s", this->ifname_.c_str(), pcap_geterr(this->p_));
  }
}

void PcapChannel::DispatchHandler(u_char* user, const pcap_pkthdr* h, const u_char* bytes) {
  PcapChannel* self = reinterpret_cast<PcapChannel*>(user);
  if (h->caplen != h->len) {
    self->Log(kLLDebug, kLCFace, "PcapChannel(%s)::DispatchHandler h->caplen %" PRIu32 " != h->len %" PRIu32 "", self->ifname_.c_str(), h->caplen, h->len);
    return;
  }
  Ptr<EtherPkt> pkt = new EtherPkt(bytes, h->len);
  self->DeliverPacket(pkt->peer(), pkt);
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
  
Ptr<DgramChannel> EtherFaceFactory::Channel(const std::string& ifname, uint16_t ether_type) {
  bool ok; int mtu; NetworkAddress local_addr;
  std::tie(ok, mtu) = this->GetIfMtu(ifname);
  if (!ok) return nullptr;
  std::tie(ok, local_addr) = this->GetIfEtherAddr(ifname);
  if (!ok) return nullptr;

  Ptr<NdnlpWireProtocol> ndnlp = this->New<NdnlpWireProtocol>(mtu);

  pcap_t* p = this->PcapOpen(ifname);
  if (p == nullptr) return nullptr;
  Ptr<PcapChannel> channel = this->New<PcapChannel>(ifname, ether_type, p, local_addr, this->av_, ndnlp);
  return channel;
}

std::tuple<bool,int> EtherFaceFactory::GetIfMtu(const std::string& ifname) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  ifreq ifr;
  strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));
  int res = ioctl(sock, SIOCGIFMTU, &ifr);
  close(sock);
  return std::forward_as_tuple(res == 0, ifr.ifr_mtu);
}

std::tuple<bool,NetworkAddress> EtherFaceFactory::GetIfEtherAddr(const std::string& ifname) {
  bool ok = false; int res;
  NetworkAddress local_addr;

#ifdef SIOCGIFHWADDR
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  ifreq ifr;
  strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name));

  res = ioctl(sock, SIOCGIFHWADDR, &ifr);
  close(sock);
  ok = (res == 0);
  if (ok) {
    local_addr.wholen = sizeof(ether_addr);
    memcpy(&local_addr.who, ifr.ifr_hwaddr.sa_data, local_addr.wholen);
  }
#else
  ifaddrs* ifa_list;
  res = getifaddrs(&ifa_list);
  if (res == 0) {  
    for (ifaddrs* ifa = ifa_list; ifa != nullptr; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr->sa_family == AF_LINK && ifname.compare(ifa->ifa_name) == 0) {
        sockaddr_dl* sa = reinterpret_cast<sockaddr_dl*>(ifa->ifa_addr);
        local_addr.wholen = sa->sdl_alen;
        memcpy(&local_addr.who, LLADDR(sa), local_addr.wholen);
        ok = true;
        break;
      }
    }
    freeifaddrs(ifa_list);
  }
#endif

  return std::forward_as_tuple(ok, local_addr);
}

pcap_t* EtherFaceFactory::PcapOpen(const std::string& ifname) {
  char errbuf[PCAP_ERRBUF_SIZE];

  errbuf[0] = '\0';
  pcap_t* p = pcap_open_live(ifname.c_str(), 65535, 0, 0, errbuf);
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
