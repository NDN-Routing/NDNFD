#include "pktconv.h"
#include <ns3/packet.h>
#include <ns3/simulator.h>
#include <ns3/ndn-header-helper.h>
namespace ndnfd {

Ptr<CcnbMessage> NdnsimPacketConverter::MessageFrom(ns3::Ptr<ns3::Packet> p) const {
  using ns3::ndn::HeaderHelper;
  
  HeaderHelper::Type type = HeaderHelper::GetNdnHeaderType(p);
  switch (type) {
    case HeaderHelper::INTEREST_NDNSIM: {
      ns3::ndn::Interest header;
      p->RemoveHeader(header);
      if (header.GetNack() > 0) {
        // ndnSIM Nack is a forwarding message indicates that face doesn't work.
        // CCN_CONTENT_NACK comes from producer indicates content isn't available.
        // They are not equivalent.
        return nullptr;
      }
      return this->InterestFrom(header);
    }
    case HeaderHelper::CONTENT_OBJECT_NDNSIM: {
      ns3::ndn::ContentObject header;
      ns3::ndn::ContentObjectTail tail;
      p->RemoveHeader(header);
      p->RemoveTrailer(tail);
      return this->ContentObjectFrom(header, p);
    }
    default: break;
  }
  return nullptr;
}

Ptr<Name> NdnsimPacketConverter::NameFrom(const ns3::ndn::Name& name) const {
  std::vector<Name::Component> comps;
  for (const std::string& comp : name.GetComponents()) {
    comps.emplace_back(reinterpret_cast<const Name::Component::value_type*>(comp.data()), comp.size());
  }
  return new Name(comps);
}

Ptr<InterestMessage> NdnsimPacketConverter::InterestFrom(const ns3::ndn::Interest& header) const {
  ccn_charbuf* c = ccn_charbuf_create();
  ccnb_element_begin(c, CCN_DTAG_Interest);

  std::basic_string<uint8_t> name_ccnb = this->NameFrom(header.GetName())->ToCcnb();
  ccn_charbuf_append(c, name_ccnb.data(), name_ccnb.size());
  
  uint8_t scope = static_cast<uint8_t>(header.GetScope());
  if (scope != 0xFF) {
    ccnb_append_tagged_binary_number(c, CCN_DTAG_Scope, scope);
  }

  ns3::Time lifetime = header.GetInterestLifetime();
  if (lifetime.IsStrictlyPositive()) {
    ccnb_append_tagged_binary_number(c, CCN_DTAG_InterestLifetime, static_cast<uint32_t>(lifetime.GetSeconds() * 4) * 1024);
  }
  
  char nonce[9];
  snprintf(nonce, sizeof(nonce), "%08x", header.GetNonce());
  ccnb_append_tagged_blob(c, CCN_DTAG_Nonce, nonce, 8);

  ccnb_element_end(c);//</Interest>
  
  Ptr<Buffer> buffer = Buffer::Adopt(&c);
  Ptr<InterestMessage> m = InterestMessage::Parse(buffer->mutable_data(), buffer->length());
  if (m != nullptr) { m->set_source_buffer(buffer); }
  return m;
}

Ptr<ContentObjectMessage> NdnsimPacketConverter::ContentObjectFrom(const ns3::ndn::ContentObject& header, ns3::Ptr<ns3::Packet> payload) const {
  ccn_charbuf* c = ccn_charbuf_create();
  ccnb_element_begin(c, CCN_DTAG_ContentObject);

  ccnb_element_begin(c, CCN_DTAG_Signature);
  ccnb_element_begin(c, CCN_DTAG_DigestAlgorithm);
  ccn_charbuf_append_tt(c, 3, CCN_UDATA);
  ccn_charbuf_append_string(c, "NOP");
  ccnb_element_end(c);//</DigestAlgorithm>
  uint64_t signature_bits[2]; signature_bits[0] = signature_bits[1] = ++const_cast<NdnsimPacketConverter*>(this)->s_;
  ccnb_append_tagged_blob(c, CCN_DTAG_SignatureBits, signature_bits, 16);
  ccnb_element_end(c);//</Signature>

  std::basic_string<uint8_t> name_ccnb = this->NameFrom(header.GetName())->ToCcnb();
  ccn_charbuf_append(c, name_ccnb.data(), name_ccnb.size());

  ccnb_element_begin(c, CCN_DTAG_SignedInfo);
  
  ccnb_append_tagged_blob(c, CCN_DTAG_PublisherPublicKeyDigest, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);

  ns3::Time timestamp = header.GetTimestamp();
  // Timestamp is not used in ndnSIM examples and is zero; use current time instead
  if (timestamp.IsZero()) timestamp = ns3::Now();
  // ccn_parse_timestamp restricts timestamp BLOB to be 3~7 bytes,
  // so timestamp must >= ns3::Seconds(16).
  ccnb_element_begin(c, CCN_DTAG_Timestamp);
  ccnb_append_timestamp_blob(c, CCN_MARKER_NONE, static_cast<intmax_t>(timestamp.GetSeconds()), static_cast<int>(timestamp.GetMicroSeconds() % 1000000));
  ccnb_element_end(c);//</Timestamp>
  
  ns3::Time freshness = header.GetFreshness();
  if (freshness.IsStrictlyPositive()) {
    ccnb_element_begin(c, CCN_DTAG_FreshnessSeconds);
    ccnb_append_number(c, static_cast<int>(freshness.GetSeconds()));
    ccnb_element_end(c);//</FreshnessSeconds>
  }

  ccnb_element_end(c);//</SignedInfo>
  
  ccnb_element_begin(c, CCN_DTAG_Content);
  uint32_t payload_size = payload->GetSize();
  ccn_charbuf_append_tt(c, payload_size, CCN_BLOB);
  uint8_t* payload_p = ccn_charbuf_reserve(c, payload_size);
  payload->CopyData(payload_p, payload_size);
  c->length += payload_size;
  ccnb_element_end(c);//</Content>

  ccnb_element_end(c);//</ContentObject>
  
  Ptr<Buffer> buffer = Buffer::Adopt(&c);
  Ptr<ContentObjectMessage> m = ContentObjectMessage::Parse(buffer->mutable_data(), buffer->length());
  if (m != nullptr) { m->set_source_buffer(buffer); }
  return m;
}

ns3::Ptr<ns3::Packet> NdnsimPacketConverter::MessageTo(Ptr<const Message> msg) const {
  if (msg->type() == InterestMessage::kType) {
    return this->InterestTo(static_cast<const InterestMessage*>(PeekPointer(msg)));
  } else if (msg->type() == ContentObjectMessage::kType) {
    return this->ContentObjectTo(static_cast<const ContentObjectMessage*>(PeekPointer(msg)));
  } else if (msg->type() == CcnbMessage::kType) {
    const CcnbMessage* ccnb = static_cast<const CcnbMessage*>(PeekPointer(msg));
    Ptr<InterestMessage> interest = InterestMessage::Parse(ccnb->msg(), ccnb->length());
    if (interest != nullptr) return this->InterestTo(interest);
    Ptr<ContentObjectMessage> co = ContentObjectMessage::Parse(ccnb->msg(), ccnb->length());
    if (co != nullptr) return this->ContentObjectTo(co);
    return nullptr;
  } else {
    return nullptr;
  }
}

ns3::Ptr<ns3::ndn::Name> NdnsimPacketConverter::NameTo(Ptr<const Name> name) const {
  ns3::Ptr<ns3::ndn::Name> n = ns3::Create<ns3::ndn::Name>();
  for (const Name::Component& comp : name->comps()) {
    n->Add(std::string(reinterpret_cast<const char*>(comp.data()), comp.size()));
  }
  return n;
}

ns3::Ptr<ns3::Packet> NdnsimPacketConverter::InterestTo(Ptr<const InterestMessage> msg) const {
  ns3::ndn::Interest header;
  // These elements are not supported by ndnSIM:
  // MinSuffixComponents MaxSuffixComponents PublisherPublicKeyDigest Exclude ChildSelector AnswerOriginKind FaceID
  
  ns3::Ptr<ns3::ndn::Name> name = this->NameTo(msg->name());
  header.SetName(name);
  
  if (msg->parsed()->scope >= 0) {
    header.SetScope(msg->parsed()->scope);
  }
  
  int lifetime = ccn_fetch_tagged_nonNegativeInteger(CCN_DTAG_InterestLifetime, msg->msg(), msg->parsed()->offset[CCN_PI_B_InterestLifetime], msg->parsed()->offset[CCN_PI_E_InterestLifetime]);
  if (lifetime >= 0) {
    header.SetInterestLifetime(ns3::Seconds(lifetime / 4096.0));
  }

  const uint8_t* nonce_blob; size_t nonce_size;
  if (0 == ccn_ref_tagged_BLOB(CCN_DTAG_Nonce, msg->msg(), msg->parsed()->offset[CCN_PI_B_Nonce], msg->parsed()->offset[CCN_PI_E_Nonce], &nonce_blob, &nonce_size) && nonce_size == 8) {
    uint32_t nonce;
    if (1 == sscanf(reinterpret_cast<const char*>(nonce_blob), "%08x", &nonce)) header.SetNonce(nonce);
  }
  
  ns3::Ptr<ns3::Packet> p = ns3::Create<ns3::Packet>();
  p->AddHeader(header);
  return p;
}

ns3::Ptr<ns3::Packet> NdnsimPacketConverter::ContentObjectTo(Ptr<const ContentObjectMessage> msg) const {
  ns3::ndn::ContentObject header;
  ns3::ndn::ContentObjectTail tail;
  // These elements are not supported by ndnSIM:
  // Signature, PublisherPublicKeyDigest, Type, FinalBlockID, KeyLocator, ExtOpt
  
  ns3::Ptr<ns3::ndn::Name> name = this->NameTo(msg->name());
  header.SetName(name);
  
  const uint8_t* timestamp_blob; size_t timestamp_size;
  if (0 == ccn_ref_tagged_BLOB(CCN_DTAG_Timestamp, msg->msg(), msg->parsed()->offset[CCN_PCO_B_Timestamp], msg->parsed()->offset[CCN_PCO_E_Timestamp], &timestamp_blob, &timestamp_size)) {
    uint64_t timestamp = 0;
    const uint8_t* timestamp_end = timestamp_blob + timestamp_size;
    for (const uint8_t* timestamp_p = timestamp_blob; timestamp_p < timestamp_end; ++timestamp_p) {
      timestamp = (timestamp << 8) + *timestamp_p;
    }
    header.SetTimestamp(ns3::Seconds(timestamp / 4096.0));
  }
  
  int freshness = ccn_fetch_tagged_nonNegativeInteger(CCN_DTAG_FreshnessSeconds, msg->msg(), msg->parsed()->offset[CCN_PCO_B_FreshnessSeconds], msg->parsed()->offset[CCN_PCO_E_FreshnessSeconds]);
  if (freshness >= 0) {
    header.SetFreshness(ns3::Seconds(freshness));
  }
  
  const uint8_t* payload; size_t payload_size;
  std::tie(payload, payload_size) = msg->payload();
  ns3::Ptr<ns3::Packet> p = ns3::Create<ns3::Packet>(payload, static_cast<uint32_t>(payload_size));
  p->AddHeader(header);
  p->AddTrailer(tail);
  return p;
}

};//namespace ndnfd
