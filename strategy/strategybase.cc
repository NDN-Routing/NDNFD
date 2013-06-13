#include "strategy.h"
extern "C" {
struct pit_face_item* send_interest(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* x, struct pit_face_item* p);
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

std::unordered_set<FaceId> StrategyBase::LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest) {
  return ie->npe()->LookupFib(interest);
}

void StrategyBase::PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds) {
  for (FaceId face : outbounds) {
    Ptr<PitUpstreamRecord> p = ie->SeekUpstream(face);
    if (p->IsExpired()) { // new PitUpstreamRecord is 'expired' when created
      p->SetExpiry(std::chrono::microseconds::zero());
      p->native()->pfi_flags &= ~CCND_PFI_UPHUNGRY;
    }
  }
}

};//namespace ndnfd
