#include "strategy_base.h"
extern "C" {
struct pit_face_item* send_interest(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* x, struct pit_face_item* p);
int face_send_queue_insert(struct ccnd_handle* h, struct face* face, struct content_entry* content);
}
#include "core/scheduler.h"
#include "face/facemgr.h"
namespace ndnfd {

void StrategyBase::SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream) {
  assert(ie != nullptr);
  assert(downstream != nullptr);
  assert(upstream != nullptr);
  upstream->set_native(send_interest(CCNDH, ie->native(), downstream->native(), upstream->native()));
}

bool StrategyBase::SendInterest(Ptr<PitEntry> ie, Ptr<PitUpstreamRecord> upstream) {
  assert(ie != nullptr);
  assert(upstream != nullptr);

  std::vector<Ptr<PitDownstreamRecord>> downstreams;//pending downstreams that does not expire soon
  for (auto it = ie->beginDownstream(); it != ie->endDownstream(); ++it) {
    Ptr<PitDownstreamRecord> p = *it;
    if (!p->IsExpired() && p->pending() && p->faceid() != upstream->faceid()) {
      downstreams.push_back(p);
    }
  };

  auto downstream_it = std::max_element(downstreams.begin(), downstreams.end(),
    [] (const Ptr<PitDownstreamRecord>& a, const Ptr<PitDownstreamRecord>& b) ->bool {
      return PitDownstreamRecord::CompareExpiry(a, b) > 0;
    });
  if (downstream_it == downstreams.end()) return false;

  Ptr<PitDownstreamRecord> downstream = *downstream_it;
  this->SendInterest(ie, downstream, upstream);
  return true;
}

void StrategyBase::SendContent(FaceId downstream, Ptr<ContentEntry> ce) {
  assert(ce != nullptr);
  Ptr<Face> face = this->global()->facemgr()->GetFace(downstream);
  if (face == nullptr) return;
  face_send_queue_insert(CCNDH, face->native(), ce->native());
}

void StrategyBase::SendNack(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, NackCode code) {
  assert(ie != nullptr);
  assert(downstream != nullptr);
  this->Log(kLLDebug, kLCStrategy, "StrategyBase::SendNack(%" PRI_PitEntrySerial ",%" PRI_FaceId ",%s)", ie->serial(), downstream->faceid(), NackCode_string(code));
  Ptr<Face> face = this->global()->facemgr()->GetFace(downstream->faceid());
  if (face == nullptr) return;

  Ptr<Buffer> buf = NackMessage::Create(code, ie->interest(), downstream->nonce());
  Ptr<CcnbMessage> message = new CcnbMessage(buf->data(), buf->length());
  message->set_source_buffer(buf);
  face->face_thread()->Send(face->id(), message);
}

void StrategyBase::SendNacks(Ptr<PitEntry> ie, NackCode code) {
  std::for_each(ie->beginDownstream(), ie->endDownstream(), std::bind(&StrategyBase::SendNack, this, ie, std::placeholders::_1, code));
}

std::unordered_set<FaceId> StrategyBase::LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest) {
  return ie->npe()->LookupFib(interest);
}

void StrategyBase::PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds) {
  for (FaceId face : outbounds) {
    Ptr<PitUpstreamRecord> p = ie->SeekUpstream(face);
    if (p->IsExpired()) { // new PitUpstreamRecord is 'expired' when created
      p->SetExpiry(std::chrono::microseconds::zero());
      p->SetFlag(CCND_PFI_UPHUNGRY, false);
    }
  }
}

};//namespace ndnfd
