#include "netface.h"
#include <net/ethernet.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/mac48-address.h>
#include "face/ndnlp.h"
#include "l3protocol.h"
#include "ndnfdsim.h"
namespace ndnfd {

NetworkAddress SimNetChannel::ConvertAddress(const ns3::Address& addr) {
  ns3::Mac48Address mac48 = ns3::Mac48Address::ConvertFrom(addr);

  NetworkAddress na;
  na.wholen = sizeof(ether_addr);
  ether_addr* ea = reinterpret_cast<ether_addr*>(&na.who);
  mac48.CopyTo(ea->ether_addr_octet);
  return na;
}

ns3::Address SimNetChannel::ConvertAddress(const NetworkAddress& addr) {
  ns3::Mac48Address mac48;
  const ether_addr* ea = reinterpret_cast<const ether_addr*>(&addr.who);
  mac48.CopyFrom(ea->ether_addr_octet);
  return mac48;
}

SimNetChannel::SimNetChannel(ns3::Ptr<ns3::NetDevice> nic, uint16_t ether_type, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) : DgramChannel(-1, SimNetChannel::ConvertAddress(nic->GetAddress()), av, wp) {
  this->ether_type_ = ether_type;
  this->nic_ = nic;
}

void SimNetChannel::Init(void) {
  this->DgramChannel::Init();
  this->nic_->GetNode()->RegisterProtocolHandler(ns3::MakeCallback(&SimNetChannel::NicReceive, this), this->ether_type_, this->nic_, true);
}

void SimNetChannel::CloseFd(void) {
  this->nic_->GetNode()->UnregisterProtocolHandler(ns3::MakeCallback(&SimNetChannel::NicReceive, this));
}

void SimNetChannel::SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  //this->Log(kLLDebug, kLCFace, "SimNetChannel::SendTo");

  ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>(pkt->data(), static_cast<uint32_t>(pkt->length()));
  this->nic_->Send(packet, SimNetChannel::ConvertAddress(peer), this->ether_type_);
}

Ptr<DgramFace> SimNetChannel::CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group) {
  Ptr<DgramFace> face = this->New<DgramFace>(this, group);
  face->set_kind(FaceKind::kMulticast);
  return face;
}

void SimNetChannel::NicReceive(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const ns3::Address& sender, const ns3::Address& receiver, ns3::NetDevice::PacketType packetType) {
  NS_ASSERT(device == this->nic_);
  NS_ASSERT(protocol == this->ether_type_);
  
  Ptr<Buffer> pkt = new Buffer(packet->GetSize());
  packet->CopyData(pkt->mutable_data(), static_cast<uint32_t>(pkt->length()));
  
  NetworkAddress sender_addr = SimNetChannel::ConvertAddress(sender);
  NetworkAddress receiver_addr = SimNetChannel::ConvertAddress(receiver);

  //this->Log(kLLDebug, kLCFace, "SimNetChannel::NicReceive");
  typedef void (DgramChannel::*DeliverMcastPacket_type)(const NetworkAddress&, const NetworkAddress&, Ptr<BufferView>);
  THIS_SIMGLOBAL->program()->ScheduleOnNextRun(std::bind((DeliverMcastPacket_type)&DgramChannel::DeliverMcastPacket, this, receiver_addr, sender_addr, pkt));
}

SimFaceFactory::SimFaceFactory(void) {
  this->av_ = new EtherAddressVerifier();
}

std::vector<ns3::Ptr<ns3::NetDevice>> SimFaceFactory::ListNICs(void) {
  std::vector<ns3::Ptr<ns3::NetDevice>> devs;
  ns3::Ptr<ns3::Node> node = THIS_SIMGLOBAL->l3()->GetObject<ns3::Node>();
  for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
    devs.push_back(node->GetDevice(i));
  }
  return devs;
}

Ptr<DgramChannel> SimFaceFactory::Channel(ns3::Ptr<ns3::NetDevice> dev, uint16_t ether_type) {
  int mtu = static_cast<int>(dev->GetMtu());
  if (mtu < 256) return nullptr;

  Ptr<NdnlpWireProtocol> ndnlp = this->New<NdnlpWireProtocol>(mtu);

  Ptr<SimNetChannel> channel = this->New<SimNetChannel>(dev, ether_type, this->av_, ndnlp);
  return channel;
}


};//namespace ndnfd
