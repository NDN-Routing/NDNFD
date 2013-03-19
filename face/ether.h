#ifndef NDNFD_FACE_ETHER_H_
#define NDNFD_FACE_ETHER_H_
#include <pcap/pcap.h>
#include "face/dgram.h"
#include "face/factory.h"
namespace ndnfd {


// A PcapChannel represents a socket using libpcap to send and receive Ethernet frames.
// Run libpcap without promisc mode.
class PcapChannel : public DgramChannel {
 public:
  PcapChannel(const std::string& ifname, uint16_t ether_type, pcap_t* p, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~PcapChannel(void) {}
  
  void set_local_ip(uint32_t value) { this->local_ip_ = value; }

 protected:
  virtual void CloseFd(void);
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  virtual void ReceiveFrom(void);

  // Joining Ethernet multicast group is triggered by joining an IPv4 multicast group
  // that is mapped to this Ethernet multicast address,
  // so group is limited to 01:00:5E:00:00:00 - 01:00:5E:7F:FF:FF.
  virtual Ptr<DgramFace> CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group);

 private:
  class EtherPkt : public BufferView {
   public:
    EtherPkt(const uint8_t* pkt, size_t length);
    virtual const uint8_t* data() const { return this->pkt_; }
    virtual size_t length(void) const { return this->length_; }
    virtual void Take(size_t n) { assert(false); }
    virtual void Pull(size_t n) { assert(false); }
    NetworkAddress peer(void) const;
    NetworkAddress local(void) const;
   private:
    const uint8_t* pkt_;
    size_t length_;
    DISALLOW_COPY_AND_ASSIGN(EtherPkt);
  };
  
  std::string ifname_;
  uint16_t ether_type_;
  pcap_t* p_;
  bpf_program filter_;
  uint32_t local_ip_;
  int ip_mcast_fd_;
  
  // DispatchHandler is called by libpcap when a packet is captured.
  // user is PcapChannel*.
  static void DispatchHandler(u_char* user, const pcap_pkthdr* h, const u_char* bytes);
  
  // Join a IPv4 multicast group on .ip_cast_fd_,
  // so that libpcap can receive Ethernet multicast frames on group.
  bool JoinIpMcast(const NetworkAddress& group);
  
  std::chrono::microseconds ScheduledReceive(void);
  
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
  
  // ListNICs returns a list of ifnames.
  std::vector<std::string> ListNICs(void);
  
  // Channel creates a DgramChannel for Ethernet.
  Ptr<DgramChannel> Channel(const std::string& ifname, uint16_t ether_type);

 private:
  Ptr<EtherAddressVerifier> av_;

  // GetIfMtu returns MTU of the NIC
  std::tuple<bool,int> GetIfMtu(const std::string& ifname);
  // GetIfAddr returns Ethernet and IPv4 address of the NIC
  std::tuple<bool,NetworkAddress,uint32_t> GetIfAddr(const std::string& ifname);
  
  // PcapOpen opens pcap handle for NIC, and set it to nonblock mode.
  pcap_t* PcapOpen(const std::string& ifname);
  
  DISALLOW_COPY_AND_ASSIGN(EtherFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ETHER_H
