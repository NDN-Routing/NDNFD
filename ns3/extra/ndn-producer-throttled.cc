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

void ProducerThrottled::OnInterest(Ptr<const Interest> interest) {
  this->App::OnInterest(interest);
  NS_LOG_FUNCTION(this << interest);
  if (!this->m_active) return;
  
  TimeValue freshness;
  this->GetAttribute("Freshness", freshness);
  
  Ptr<ContentObject> co = Create<ContentObject>();
  co->SetName(interest->GetName());
  co->SetFreshness(freshness.Get());
  
  uint32_t pending_i = ++this->pending_i_;
  this->pendings_[pending_i] = co;
  this->pd_->SubmitJob(UintegerValue(pending_i));
}

void ProducerThrottled::ProcessComplete(ProcessingDelay::Job job) {
  Ptr<const UintegerValue> pending_ptr = static_cast<const UintegerValue*>(&job);
  uint32_t pending_i = static_cast<uint32_t>(pending_ptr->Get());
  auto pending_it = this->pendings_.find(pending_i);
  NS_ASSERT(pending_it != this->pendings_.end());
  Ptr<ContentObject> co = pending_it->second;
  this->pendings_.erase(pending_it);
  
  if (!this->m_active) return;//app stopped

  NS_LOG_INFO("node("<< GetNode()->GetId() <<") responding with ContentObject: " << co->GetName());
  
  UintegerValue virtual_payload_size;
  this->GetAttribute("PayloadSize", virtual_payload_size);
  Ptr<Packet> packet = Create<Packet>(virtual_payload_size.Get());
  co->SetPayload(packet);

  this->m_face->ReceiveData(co);
  this->m_transmittedContentObjects(co, this, this->m_face);
}

};//namespace ndn
};//namespace ndnfd
