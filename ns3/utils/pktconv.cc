#include "pktconv.h"
#include <ns3/packet.h>
#include <ns3/simulator.h>
#include <ns3/ndn-wire.h>
namespace ndnfd {

using ns3::ndn::Wire;

Ptr<Name> NdnsimPacketConverter::NameFrom(ns3::Ptr<const ns3::ndn::Name> name) const {
  std::vector<Name::Component> comps;
  for (auto comp : *name) {
    comps.emplace_back(reinterpret_cast<const Name::Component::value_type*>(comp.buf()), comp.size());
  }
  return new Name(comps);
}

Ptr<InterestMessage> NdnsimPacketConverter::InterestFrom(ns3::Ptr<const ns3::ndn::Interest> interest) const {
  assert(interest->GetNack() == ns3::ndn::Interest::NORMAL_INTEREST);
  ns3::Ptr<ns3::Packet> pkt = Wire::FromInterest(interest, Wire::WIRE_FORMAT_CCNB);
  Ptr<Buffer> buf = new Buffer(pkt->GetSize());
  pkt->CopyData(buf->mutable_data(), buf->length());
  Ptr<InterestMessage> m = InterestMessage::Parse(buf->data(), buf->length());
  if (m != nullptr) { m->set_source_buffer(buf); }
  return m;
}

Ptr<ContentObjectMessage> NdnsimPacketConverter::ContentObjectFrom(ns3::Ptr<const ns3::ndn::ContentObject> co) const {
  ns3::Ptr<ns3::Packet> pkt = Wire::FromData(co, Wire::WIRE_FORMAT_CCNB);
  Ptr<Buffer> buf = new Buffer(pkt->GetSize());
  pkt->CopyData(buf->mutable_data(), buf->length());
  Ptr<ContentObjectMessage> m = ContentObjectMessage::Parse(buf->data(), buf->length());
  if (m != nullptr) { m->set_source_buffer(buf); }
  return m;
}

NackCode NdnsimPacketConverter::NackCodeFrom(uint8_t code) const {
  switch (code) {
    case ns3::ndn::Interest::NACK_LOOP      : return NackCode::kDuplicate;
    case ns3::ndn::Interest::NACK_CONGESTION: return NackCode::kCongestion;
    case ns3::ndn::Interest::NACK_GIVEUP_PIT: return NackCode::kNoData;
  }
  return static_cast<NackCode>(0);
}

Ptr<NackMessage> NdnsimPacketConverter::NackFrom(ns3::Ptr<const ns3::ndn::Interest> nack) const {
  NackCode code = this->NackCodeFrom(nack->GetNack());
  assert(code != static_cast<NackCode>(0));
  
  ns3::ndn::Interest* nack_interest = const_cast<ns3::ndn::Interest*>(ns3::PeekPointer(nack));
  uint8_t nack_code = nack_interest->GetNack();
  nack_interest->SetNack(ns3::ndn::Interest::NORMAL_INTEREST);  
  Ptr<InterestMessage> interest = this->InterestFrom(nack_interest);
  nack_interest->SetNack(nack_code);
  if (interest == nullptr) return nullptr;

  Ptr<Buffer> buf = NackMessage::Create(code, interest, InterestMessage::Nonce { nullptr, 0 });
  Ptr<NackMessage> m = NackMessage::Parse(buf->data(), buf->length());
  if (m != nullptr) { m->set_source_buffer(buf); }
  return m;
}

ns3::Ptr<ns3::ndn::Name> NdnsimPacketConverter::NameTo(Ptr<const Name> name) const {
  ns3::Ptr<ns3::ndn::Name> n = ns3::Create<ns3::ndn::Name>();
  for (const Name::Component& comp : name->comps()) {
    n->append(comp.data(), comp.size());
  }
  return n;
}

ns3::Ptr<ns3::ndn::Interest> NdnsimPacketConverter::InterestTo(Ptr<const InterestMessage> msg) const {
  ns3::Ptr<ns3::Packet> pkt = ns3::Create<ns3::Packet>(msg->msg(), msg->length());
  return Wire::ToInterest(pkt, Wire::WIRE_FORMAT_CCNB);
}

ns3::Ptr<ns3::ndn::ContentObject> NdnsimPacketConverter::ContentObjectTo(Ptr<const ContentObjectMessage> msg) const {
  ns3::Ptr<ns3::Packet> pkt = ns3::Create<ns3::Packet>(msg->msg(), msg->length());
  return Wire::ToData(pkt, Wire::WIRE_FORMAT_CCNB);
}

uint8_t NdnsimPacketConverter::NackCodeTo(NackCode code) const {
  switch (code) {
    case NackCode::kDuplicate : return ns3::ndn::Interest::NACK_LOOP;
    case NackCode::kCongestion: return ns3::ndn::Interest::NACK_CONGESTION;
    case NackCode::kNoData    : return ns3::ndn::Interest::NACK_GIVEUP_PIT;
  }
  return ns3::ndn::Interest::NORMAL_INTEREST;
}

ns3::Ptr<ns3::ndn::Interest> NdnsimPacketConverter::NackTo(Ptr<const NackMessage> msg) const {
  ns3::Ptr<ns3::ndn::Interest> interest = this->InterestTo(msg->interest());
  if (interest == nullptr) return nullptr;
  interest->SetNack(this->NackCodeTo(msg->code()));
  return interest;
}

};//namespace ndnfd
