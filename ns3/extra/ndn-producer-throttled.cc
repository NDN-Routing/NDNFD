#include "ndn-producer-throttled.h"
#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/uinteger.h>
#include <ns3/packet.h>
#include <ns3/ndn-interest.h>
#include <ns3/ndn-app-face.h>
namespace ns3 {
namespace ndn {

NS_LOG_COMPONENT_DEFINE("ndn.ProducerThrottled");

TypeId ProducerThrottled::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::ndn::ProducerThrottled")
    .SetParent<Producer>()
    .AddConstructor<ProducerThrottled>()
    .AddAttribute("ProcessingDelay", "processing delay model",
      PointerValue(),
      MakePointerAccessor(&ProducerThrottled::SetProcessingDelay, &ProducerThrottled::GetProcessingDelay),
      MakePointerChecker<ProcessingDelay>())
    ;
  return tid;
}

ProducerThrottled::ProducerThrottled(void) : pending_i_(0) {
  this->SetProcessingDelay(CreateObject<ProcessingDelay>());
}

void ProducerThrottled::SetProcessingDelay(Ptr<ProcessingDelay> value) {
  this->pd_ = value;
  this->pd_->SetCompleteCallback(MakeCallback(&ProducerThrottled::ProcessComplete, this));
}

void ProducerThrottled::OnInterest(const Ptr<const Interest>& interest, Ptr<Packet> packet) {
  this->App::OnInterest(interest, packet);
  NS_LOG_FUNCTION(this << interest);
  if (!this->m_active) return;
  
  TimeValue freshness;
  this->GetAttribute("Freshness", freshness);
  
  Ptr<ContentObject> header = Create<ContentObject>();
  header->SetName(Create<Name>(interest->GetName()));
  header->SetFreshness(freshness.Get());
  
  uint32_t pending_i = ++this->pending_i_;
  this->pendings_[pending_i] = header;
  this->pd_->SubmitJob(UintegerValue(pending_i));
}

void ProducerThrottled::ProcessComplete(ProcessingDelay::Job job) {
  Ptr<const UintegerValue> pending_ptr = static_cast<const UintegerValue*>(&job);
  uint32_t pending_i = static_cast<uint32_t>(pending_ptr->Get());
  auto pending_it = this->pendings_.find(pending_i);
  NS_ASSERT(pending_it != this->pendings_.end());
  Ptr<ContentObject> header = pending_it->second;
  this->pendings_.erase(pending_it);

  NS_LOG_INFO("node("<< GetNode()->GetId() <<") responding with ContentObject:\n" << boost::cref(*header));
  
  UintegerValue virtual_payload_size;
  this->GetAttribute("PayloadSize", virtual_payload_size);

  Ptr<Packet> packet = Create<Packet>(virtual_payload_size.Get());
  packet->AddHeader(*header);
  static ContentObjectTail tail;
  packet->AddTrailer(tail);

  this->m_protocolHandler(packet);
  this->m_transmittedContentObjects(header, packet, this, this->m_face);
}

};//namespace ndn
};//namespace ndnfd
