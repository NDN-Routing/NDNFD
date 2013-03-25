#include "netface.h"
#include <net/ethernet.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/mac48-address.h>
namespace ndnfd {

const uint16_t SimNetChannel::EtherProtocol = ns3::ndn::L3Protocol::ETHERNET_FRAME_TYPE;

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

SimNetChannel::SimNetChannel(ns3::Ptr<ns3::NetDevice> nic, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) : DgramChannel(-1, SimNetChannel::ConvertAddress(nic->GetAddress()), av, wp) {
  this->nic_ = nic;
}

void SimNetChannel::Init(void) {
  this->nic_->GetNode()->RegisterProtocolHandler(ns3::MakeCallback(&SimNetChannel::NicReceive, this), SimNetChannel::EtherProtocol, this->nic_, true);
}

void SimNetChannel::CloseFd(void) {
  this->nic_->GetNode()->UnregisterProtocolHandler(ns3::MakeCallback(&SimNetChannel::NicReceive, this));
}

void SimNetChannel::SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>(pkt->data(), static_cast<uint32_t>(pkt->length()));
  this->nic_->Send(packet, SimNetChannel::ConvertAddress(peer), SimNetChannel::EtherProtocol);
}

Ptr<DgramFace> SimNetChannel::CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group) {
  Ptr<DgramFace> face = this->New<DgramFace>(this, group);
  face->set_kind(FaceKind::kMulticast);
  return face;
}

void SimNetChannel::NicReceive(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const ns3::Address& sender, const ns3::Address& receiver, ns3::NetDevice::PacketType packetType) {
  NS_ASSERT(device == this->nic_);
  NS_ASSERT(protocol == SimNetChannel::EtherProtocol);
  Ptr<Buffer> pkt = new Buffer(packet->GetSize());
  packet->CopyData(pkt->mutable_data(), static_cast<uint32_t>(pkt->length()));
  
  NetworkAddress sender_addr = SimNetChannel::ConvertAddress(sender);
  NetworkAddress receiver_addr = SimNetChannel::ConvertAddress(receiver);
  this->DeliverMcastPacket(receiver_addr, sender_addr, pkt);
}


};//namespace ndnfd
