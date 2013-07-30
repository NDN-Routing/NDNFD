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
  /*
  ns3::Ptr<ns3::Packet> pkt = Wire::FromData(co, Wire::WIRE_FORMAT_CCNB);
  Ptr<Buffer> buf = new Buffer(pkt->GetSize());
  pkt->CopyData(buf->mutable_data(), buf->length());
  Ptr<ContentObjectMessage> m = ContentObjectMessage::Parse(buf->data(), buf->length());
  if (m != nullptr) { m->set_source_buffer(buf); }
  return m;
  */
  // libccn requires SignatureBits to have at least 16 octets, but Wire::FromData writes only 4 octets

  ccn_charbuf* c = ccn_charbuf_create();
  ccnb_element_begin(c, CCN_DTAG_ContentObject);

  ccnb_element_begin(c, CCN_DTAG_Signature);
  ccnb_tagged_putf(c, CCN_DTAG_DigestAlgorithm, "NOP");
  uint64_t signature_bits[2]; signature_bits[0] = signature_bits[1] = ++const_cast<NdnsimPacketConverter*>(this)->s_;
  ccnb_append_tagged_blob(c, CCN_DTAG_SignatureBits, signature_bits, 16);
  ccnb_element_end(c);//</Signature>

  std::basic_string<uint8_t> name_ccnb = this->NameFrom(co->GetNamePtr())->ToCcnb();
  ccn_charbuf_append(c, name_ccnb.data(), name_ccnb.size());

  ccnb_element_begin(c, CCN_DTAG_SignedInfo);
  
  ccnb_append_tagged_blob(c, CCN_DTAG_PublisherPublicKeyDigest, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);

  ns3::Time timestamp = co->GetTimestamp();
  // Timestamp is not used in ndnSIM examples and is zero; use current time instead
  if (timestamp.IsZero()) timestamp = ns3::Now();
  // ccn_parse_timestamp restricts timestamp BLOB to be 3~7 bytes,
  // so timestamp must >= ns3::Seconds(16).
  ccnb_element_begin(c, CCN_DTAG_Timestamp);
  ccnb_append_timestamp_blob(c, CCN_MARKER_NONE, static_cast<intmax_t>(timestamp.GetSeconds()), static_cast<int>(timestamp.GetMicroSeconds() % 1000000));
  ccnb_element_end(c);//</Timestamp>
  
  ns3::Time freshness = co->GetFreshness();
  if (freshness.IsStrictlyPositive()) {
    ccnb_element_begin(c, CCN_DTAG_FreshnessSeconds);
    ccnb_append_number(c, static_cast<int>(freshness.GetSeconds()));
    ccnb_element_end(c);//</FreshnessSeconds>
  }

  ccnb_element_end(c);//</SignedInfo>
  
  ns3::Ptr<const ns3::Packet> payload = co->GetPayload();
  ccnb_element_begin(c, CCN_DTAG_Content);
  uint32_t payload_size = payload->GetSize();
  ccn_charbuf_append_tt(c, payload_size, CCN_BLOB);
  uint8_t* payload_p = ccn_charbuf_reserve(c, payload_size);
  payload->CopyData(payload_p, payload_size);
  c->length += payload_size;
  ccnb_element_end(c);//</Content>

  ccnb_element_end(c);//</ContentObject>

  // TODO signature, keylocator
  
  Ptr<Buffer> buffer = Buffer::Adopt(&c);
  Ptr<ContentObjectMessage> m = ContentObjectMessage::Parse(buffer->mutable_data(), buffer->length());
  if (m != nullptr) { m->set_source_buffer(buffer); }
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
  /*
  ns3::Ptr<ns3::Packet> pkt = ns3::Create<ns3::Packet>(msg->msg(), msg->length());
  return Wire::ToData(pkt, Wire::WIRE_FORMAT_CCNB);
  */
  // There is a BLOB header in <Content> element, but Wire::ToData treats it as part of payload.

  ns3::Ptr<ns3::ndn::ContentObject> co = ns3::Create<ns3::ndn::ContentObject>();
  co->SetName(this->NameTo(msg->name()));
  
  const uint8_t* timestamp_blob; size_t timestamp_size;
  if (0 == ccn_ref_tagged_BLOB(CCN_DTAG_Timestamp, msg->msg(), msg->parsed()->offset[CCN_PCO_B_Timestamp], msg->parsed()->offset[CCN_PCO_E_Timestamp], &timestamp_blob, &timestamp_size)) {
    uint64_t timestamp = 0;
    const uint8_t* timestamp_end = timestamp_blob + timestamp_size;
    for (const uint8_t* timestamp_p = timestamp_blob; timestamp_p < timestamp_end; ++timestamp_p) {
      timestamp = (timestamp << 8) + *timestamp_p;
    }
    co->SetTimestamp(ns3::Seconds(timestamp / 4096.0));
  }
  
  int freshness = ccn_fetch_tagged_nonNegativeInteger(CCN_DTAG_FreshnessSeconds, msg->msg(), msg->parsed()->offset[CCN_PCO_B_FreshnessSeconds], msg->parsed()->offset[CCN_PCO_E_FreshnessSeconds]);
  if (freshness >= 0) {
    co->SetFreshness(ns3::Seconds(freshness));
  }
  
  const uint8_t* payload; size_t payload_size;
  std::tie(payload, payload_size) = msg->payload();
  co->SetPayload(ns3::Create<ns3::Packet>(payload, static_cast<uint32_t>(payload_size)));
  
  // TODO signature, keylocator

  return co;
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
