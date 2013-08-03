#include "netface.h"
#include <net/ethernet.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/mac48-address.h>
#include "message/ccnb.h"
#include "l3protocol.h"
#include "ndnfdsim.h"
#include "../helper/hop_count.h"
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

SimNetChannel::SimNetChannel(ns3::Ptr<ns3::NetDevice> nic, uint16_t ether_type, Ptr<const AddressVerifier> av, Ptr<const WireProtocol> wp) : DgramChannel(-1, SimNetChannel::ConvertAddress(nic->GetAddress()), av, wp) {
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
  //this->Log(kLLDebug, kLCFace, "SimNetChannel::Send src=%s dst=%s length=%" PRIuMAX "", this->av()->ToString(this->local_addr()).c_str(), this->av()->ToString(peer).c_str(), static_cast<uintmax_t>(pkt->length()));

  ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>(pkt->data(), static_cast<uint32_t>(pkt->length()));
  this->nic_->Send(packet, SimNetChannel::ConvertAddress(peer), this->ether_type_);
}

Ptr<DgramFace> SimNetChannel::CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group) {
  Ptr<DgramFace> face = this->New<DgramFace>(this, group);
  face->set_kind(FaceKind::kMulticast);
  return face;
}

void SimNetChannel::FaceSend(Ptr<DgramFace> face, Ptr<const Message> message) {
  this->DgramChannel::FaceSend(face, message);
  // TODO trace network usage
}

void SimNetChannel::DeliverMessage(Ptr<DgramFace> face, Ptr<Message> message) {
  // TODO trace network usage
  if (message->type() == ContentObjectMessage::kType) {
    // note: this would invalidate explicit digest, but ContentStore won't notice
    SimHopCount::Increment(const_cast<ContentObjectMessage*>(static_cast<const ContentObjectMessage*>(PeekPointer(message))));
  }
  this->DgramChannel::DeliverMessage(face, message);
}

void SimNetChannel::NicReceive(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const ns3::Address& sender, const ns3::Address& receiver, ns3::NetDevice::PacketType packetType) {
  NS_ASSERT(device == this->nic_);
  NS_ASSERT(protocol == this->ether_type_);
  
  Ptr<Buffer> pkt = new Buffer(packet->GetSize());
  packet->CopyData(pkt->mutable_data(), static_cast<uint32_t>(pkt->length()));
  
  NetworkAddress sender_addr = SimNetChannel::ConvertAddress(sender);
  NetworkAddress receiver_addr = SimNetChannel::ConvertAddress(receiver);
  //this->Log(kLLDebug, kLCFace, "SimNetChannel::NicReceive src=%s dst=%s length=%" PRIuMAX "", this->av()->ToString(sender_addr).c_str(), this->av()->ToString(receiver_addr).c_str(), static_cast<uintmax_t>(pkt->length()));
  
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
    //this->Log(kLLDebug, kLCSim, "SimFaceFactory::ListNICs %" PRIxPTR "", PeekPointer(devs.back()));
  }
  //this->Log(kLLDebug, kLCSim, "SimFaceFactory::ListNICs %" PRIuMAX "", static_cast<uintmax_t>(devs.size()));
  return devs;
}

Ptr<DgramChannel> SimFaceFactory::Channel(ns3::Ptr<ns3::NetDevice> dev, uint16_t ether_type) {
  //int mtu = static_cast<int>(dev->GetMtu());
  //if (mtu < 256) return nullptr;
  dev->SetMtu(32767);

  //Ptr<WireProtocol> wp = this->New<NdnlpWireProtocol>(mtu);
  Ptr<WireProtocol> wp = this->New<CcnbWireProtocol>(CcnbWireProtocol::Mode::kDgramIgnoreTail);
  
  Ptr<SimNetChannel> channel = this->New<SimNetChannel>(dev, ether_type, this->av_, wp);
  return channel;
}


};//namespace ndnfd
