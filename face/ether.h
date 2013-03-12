#ifndef NDNFD_FACE_ETHER_H_
#define NDNFD_FACE_ETHER_H_
#include <pcap/pcap.h>
#include "face/dgram.h"
#include "face/factory.h"
namespace ndnfd {

// A PcapChannel represents a socket using libpcap
// to send and receive Ethernet frames.
class PcapChannel : public DgramChannel {
 public:
  PcapChannel(const std::string& ifname, uint16_t ether_type, pcap_t* p, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~PcapChannel(void) {}

 protected:
  virtual void CloseFd(void);
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  virtual void ReceiveFrom(void);

 private:
  class EtherPkt : public BufferView {
   public:
    EtherPkt(const uint8_t* pkt, size_t length);
    virtual const uint8_t* data() const { return this->pkt_; }
    virtual size_t length(void) const { return this->length_; }
    virtual void Take(size_t n) { assert(false); }
    virtual void Pull(size_t n) { assert(false); }
    NetworkAddress peer(void) const;
   private:
    const uint8_t* pkt_;
    size_t length_;
  };
  
  std::string ifname_;
  uint16_t ether_type_;
  pcap_t* p_;
  bpf_program filter_;
  
  // DispatchHandler is called by libpcap when a packet is captured.
  // user is PcapChannel*.
  static void DispatchHandler(u_char* user, const pcap_pkthdr* h, const u_char* bytes);
  
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
// Ethernet faces always use NdnlpWireProtocol.
class EtherFaceFactory : public FaceFactory {
 public:
  EtherFaceFactory(void);
  virtual ~EtherFaceFactory(void) {}
  
  // Channel creates a DgramChannel for Ethernet.
  Ptr<DgramChannel> Channel(const std::string& ifname, uint16_t ether_type);

 private:
  Ptr<EtherAddressVerifier> av_;

  // GetIfMtu returns MTU of the NIC
  std::tuple<bool,int> GetIfMtu(const std::string& ifname);
  // GetIfEtherAddr returns Ethernet address of the NIC
  std::tuple<bool,NetworkAddress> GetIfEtherAddr(const std::string& ifname);
  
  // PcapOpen opens pcap handle for NIC, and set it to nonblock mode.
  pcap_t* PcapOpen(const std::string& ifname);
  
  DISALLOW_COPY_AND_ASSIGN(EtherFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ETHER_H
