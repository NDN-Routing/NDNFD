#ifndef NDNFD_FACE_ETHER_H_
#define NDNFD_FACE_ETHER_H_
#include <pcap.h>
#include "face/dgram.h"
#include "face/factory.h"
namespace ndnfd {

// A PcapChannel represents a socket using libpcap
// to send and receive Ethernet frames.
class PcapChannel : public DgramChannel {
 public:
  PcapChannel(std::string ifname, uint16_t ether_type, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~PcapChannel(void) {}

 protected:
  virtual void RegisterPoll(void) {}
  virtual void CloseFd(void);
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  virtual void ReceiveFrom(void);

 private:
  std::string ifname_;
  uint16_t ether_type_;
  pcap_t* p_;
  bpf_program filter_;
  char errbuf_[PCAP_ERRBUF_SIZE];
  
  void ClearPcapError(void) { this->errbuf_[0] = '\0'; }
  bool HasPcapError(void) const { return this->errbuf_[0] != '\0'; }
  
  void PcapHandler(u_char* user, const pcap_pkthdr* h, const u_char* bytes);
  
  DISALLOW_COPY_AND_ASSIGN(PcapChannel);
};

// A EtherAddressVerifier verifies Ethernet addresses.
// Address is struct ether_addr.
class EtherAddressVerifier : public AddressVerifier {
 public:
  EtherAddressVerifier(void) {}
  virtual ~EtherAddressVerifier(void) {}
  virtual bool Check(const NetworkAddress& addr);
  virtual AddressHashKey GetHashKey(const NetworkAddress& addr);
  virtual std::string ToString(const NetworkAddress& addr);
  static std::tuple<bool,NetworkAddress> Parse(std::string s);
 private:
  DISALLOW_COPY_AND_ASSIGN(EtherAddressVerifier);
};

// A EtherFaceFactory creates Face objects for Ethernet.
class EtherFaceFactory : public FaceFactory {
 public:
  EtherFaceFactory(Ptr<WireProtocol> wp);
  virtual ~EtherFaceFactory(void) {}
  
  // Channel creates a DgramChannel for Ethernet.
  Ptr<DgramChannel> Channel(std::string ifname, uint16_t ether_type);

 private:
  Ptr<EtherAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(EtherFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ETHER_H
