#include "floodfirst.h"
#include "face/facemgr.h"
namespace ndnfd {

StrategyType_def(FloodFirstStrategy, floodfirst);

void FloodFirstStrategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  if (ie->beginUpstream() != ie->endUpstream()) {
    // has FIB match
    this->OriginalStrategy::PropagateNewInterest(ie);
    return;
  }
  // no FIB match, flood

  // collect downstreams
  std::vector<Ptr<Face>> downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    Ptr<Face> downstream = this->global()->facemgr()->GetFace(p->faceid());
    if (downstream != nullptr) downstreams.push_back(downstream);
  });
  
  // build outbound list
  std::unordered_set<FaceId> outbounds;
  for (Ptr<Face> face : *this->global()->facemgr()) {
    if (face->kind() == FaceKind::kMulticast && face->CanSend()) {
      // exclude those that can reach unicast peer
      bool reach = false;
      for (Ptr<Face> downstream : downstreams) {
        if (face->SendReachable(downstream)) {
          reach = true;
          break;
        }
      }
      if (reach) continue;
      outbounds.insert(face->id());
    }
  };
  if (outbounds.empty()) return;
  this->Log(kLLDebug, kLCStrategy, "FloodFirstStrategy::PropagateNewInterest(%" PRI_PitEntrySerial ") flood", ie->serial());
  
  this->PopulateOutbounds(ie, outbounds);
  this->SchedulePropagate(ie, ie->NextEventDelay(true));
}

void FloodFirstStrategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
  Ptr<Face> inface = this->global()->facemgr()->GetFace(co->incoming_face());
  if (inface == nullptr) return;
  Ptr<Face> peer = inface;
  if (inface->kind() == FaceKind::kMulticast) {
    peer = this->global()->facemgr()->MakeUnicastFace(inface, co->incoming_sender());
  }

  if (matching_suffix >= 0 && matching_suffix < 2) {
    this->Log(kLLDebug, kLCStrategy, "FloodFirstStrategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId " matching_suffix=%d", npe->name()->ToUri().c_str(), peer->id(), matching_suffix);
    npe->GetStrategyExtra<OriginalStrategy::NpeExtra>()->UpdateBestFace(peer->id());
  }
  
  if (this->fib_prefix_comps() == npe->name()->n_comps()) {
    this->Log(kLLDebug, kLCStrategy, "FloodFirstStrategy::DidSatisfyPendingInterests(%s) forwarding=%" PRI_FaceId "", npe->name()->ToUri().c_str(), peer->id());
    Ptr<ForwardingEntry> forw = npe->SeekForwarding(peer->id());
    forw->Refresh(this->fib_entry_expires());
  }
}

// TODO AgeBestFace; OriginalStrategy::AgeBestFace doesn't apply because extra type doesn't match

};//namespace ndnfd
