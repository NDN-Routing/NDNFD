#include "ether.h"
#include <net/ethernet.h>
#ifdef __linux__
#include <netinet/ether.h>
#endif
#include "util/endian.h"

namespace ndnfd {

PcapChannel::PcapChannel(std::string ifname, uint16_t ether_type, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) : DgramChannel(-1, local_addr, av, wp){
  this->ifname_ = ifname;
  this->p_ = nullptr;
  this->ether_type_ = ether_type;
}

void PcapChannel::Init(void) {
  this->ClearPcapError();
  this->p_ = pcap_open_live(this->ifname_.c_str(), 65535, 0, 0, this->errbuf_);
  if (this->p_ == nullptr && this->HasPcapError()) {
    this->Log(kLLError, kLCFace, "PcapChannel(%s)::Init pcap_open_live error %s", this->ifname_.c_str(), this->errbuf_);
    return;
  } else if (this->HasPcapError()) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::Init pcap_open_live %s", this->ifname_.c_str(), this->errbuf_);
  } else {
    this->Log(kLLInfo, kLCFace, "PcapChannel(%s)::Init pcap_open_live OK", this->ifname_.c_str());
  }
  
  int res;
  this->ClearPcapError();
  res = pcap_setnonblock(this->p_, 1, this->errbuf_);
  if (res != 0) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::Init pcap_set_nonblock %s", this->ifname_.c_str(), this->errbuf_);
  }
  
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
  pcap_close(this->p_);
  pcap_freecode(&this->filter_);
}

void PcapChannel::SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  ether_header* hdr = reinterpret_cast<ether_header*>(pkt->Push(sizeof(ether_header)));
  memcpy(hdr->ether_dhost, reinterpret_cast<const ether_addr*>(&peer.who)->ether_addr_octet, sizeof(hdr->ether_dhost));
  //TODO set hdr->ether_shost to correct value
  hdr->ether_type = htobe16(this->ether_type_);
  int res = pcap_inject(this->p_, pkt->data(), pkt->length());
  if (res == -1) {
    this->Log(kLLWarn, kLCFace, "PcapChannel(%s)::SendTo pcap_inject %s", this->ifname_.c_str(), pcap_geterr(this->p_));
  }
}

void PcapChannel::ReceiveFrom(void) {
}

void PcapChannel::PcapHandler(u_char* user, const pcap_pkthdr* h, const u_char* bytes) {
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

EtherFaceFactory::EtherFaceFactory(Ptr<WireProtocol> wp) : FaceFactory(wp) {
  this->av_ = new EtherAddressVerifier();
}
  
Ptr<DgramChannel> EtherFaceFactory::Channel(std::string ifname, uint16_t ether_type) {
  return this->New<PcapChannel>(ifname, ether_type, NetworkAddress(), this->av_, this->wp());
}

};//namespace ndnfd
