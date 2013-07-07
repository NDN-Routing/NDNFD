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

void StrategyBase::SendContent(FaceId downstream, Ptr<ContentEntry> ce) {
  assert(ce != nullptr);
  Ptr<Face> face = this->global()->facemgr()->GetFace(downstream);
  if (face == nullptr) return;
  face_send_queue_insert(CCNDH, face->native(), ce->native());
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
