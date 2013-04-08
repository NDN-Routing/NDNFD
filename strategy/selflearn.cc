#include "selflearn.h"
#include <algorithm>
#include "face/facemgr.h"
#include "core/scheduler.h"
namespace ndnfd {

std::unordered_set<FaceId> SelfLearnStrategy::LookupOutbounds(Ptr<PitEntry> ie, Ptr<InterestMessage> interest) {
  Ptr<NamePrefixEntry> npe = ie->npe();
  
  // collect downstreams
  std::vector<Ptr<Face>> downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    Ptr<Face> downstream = this->global()->facemgr()->GetFace(p->faceid());
    if (downstream != nullptr) downstreams.push_back(downstream);
  });

  // lookup FIB
  std::unordered_set<FaceId> candidates = npe->LookupFib(interest);
  // also include (inherited) best face
  candidates.insert(npe->GetBestFace());
  candidates.insert(npe->prev_faceid());
  candidates.erase(FaceId_none);
  std::unordered_set<FaceId> outbounds;

  // select outbounds
  for (FaceId outbound : candidates) {
    Ptr<Face> upstream = this->global()->facemgr()->GetFace(outbound);
    if (upstream == nullptr || !upstream->CanSend()) continue;
    bool reach = false;
    for (Ptr<Face> downstream : downstreams) {
      // if outbound unicast is a downstream: it's a consumer
      // if outbound mcast is a downstream: potential producer has seen the Interest
      // if outbound unicast is reachable on a downstream mcast face: potential producer has seen the Interest
      if (downstream->id() == outbound || downstream->SendReachable(upstream)) {
        reach = true;
        break;
      }
    }
    if (!reach) {
      outbounds.insert(outbound);
    }
  }
  
  return outbounds;
}

void SelfLearnStrategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  if (ie->beginUpstream() == ie->endUpstream()) {
    // not outbound, need broadcast
    this->StartFlood(ie);
  } else {
    this->Strategy::PropagateNewInterest(ie);
  }
}

void SelfLearnStrategy::DidnotArriveOnBestFace(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidnotArriveOnBestFace(%" PRI_PitEntrySerial ") face=%" PRI_FaceId "", ie->serial(), ie->npe()->best_faceid());
  for (Ptr<NamePrefixEntry> npe1 = ie->npe(); npe1 != nullptr; npe1 = npe1->Parent()) {
    npe1->AdjustPredictUp();
  }
  this->StartFlood(ie);
}

void SelfLearnStrategy::StartFlood(Ptr<PitEntry> ie) {
  // Every upstream tried so far is listed in the PIT entry.
  // That list might be empty, if there's no FIB entry for the prefix.
  // We need to figure out which faces to broadcast, set expiry time to zero,
  // so that DoPropagate will forward to them all at once.

  // collect downstreams and upstreams
  std::vector<Ptr<Face>> downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    Ptr<Face> downstream = this->global()->facemgr()->GetFace(p->faceid());
    if (downstream != nullptr) downstreams.push_back(downstream);
  });
  std::unordered_map<FaceId, Ptr<Face>> upstreams;
  std::for_each(ie->beginUpstream(), ie->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
    Ptr<Face> upstream = this->global()->facemgr()->GetFace(p->faceid());
    if (upstream != nullptr) upstreams[upstream->id()] = upstream;
  });
  
  std::string debug_list; char debug_buf[32];
#define DEBUG_APPEND_FaceId(x) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ",", x); debug_list.append(debug_buf); }

  // find faces to broadcast
  std::vector<Ptr<Face>> mcast_outbound, unicast_outbound;
  std::unordered_set<FaceId> outbounds;
  for (Ptr<Face> face : *this->global()->facemgr()) {
    if (!face->CanSend() || upstreams.count(face->id()) != 0) {
      // exclude no-send faces, and faces already tried earlier
      continue;
    }
    switch (face->kind()) {
      case FaceKind::kInternal:
      case FaceKind::kApp:
        // exclude local faces: they always register prefix
        continue;
      case FaceKind::kMulticast:
        // include mcast faces
        mcast_outbound.push_back(face);
        outbounds.insert(face->id());
        DEBUG_APPEND_FaceId(face->id());
        break;
      case FaceKind::kUnicast:
        // tentatively include unicast faces
        unicast_outbound.push_back(face);
        break;
      default:
        assert(false);
    }
  };
  
  // exclude unicast peers reachable on mcast group
  for (Ptr<Face> face : unicast_outbound) {
    bool reach = false;
    for (auto pair : upstreams) {
      Ptr<Face> upstream = pair.second;
      if (upstream->SendReachable(face)) {
        reach = true;
        break;
      }
    }
    if (reach) continue;
    for (Ptr<Face> upstream : mcast_outbound) {
      if (upstream->SendReachable(face)) {
        reach = true;
        break;
      }
    }
    if (reach) continue;
    outbounds.insert(face->id());
    DEBUG_APPEND_FaceId(face->id());
  }
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::StartFlood(%" PRI_PitEntrySerial ") broadcast=[%s]", ie->serial(), debug_list.c_str());

  if (outbounds.empty()) return;
  
  this->PopulateOutbounds(ie, outbounds);
  this->global()->scheduler()->Schedule(ie->NextEventDelay(true), std::bind(&Strategy::DoPropagate, this, ie), &ie->ie()->ev, true);
}

void SelfLearnStrategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
  Ptr<Face> inface = this->global()->facemgr()->GetFace(co->incoming_face());
  if (inface == nullptr) return;
  Ptr<Face> peer = inface;
  if (inface->kind() == FaceKind::kMulticast) {
    peer = this->global()->facemgr()->MakeUnicastFace(inface, co->incoming_sender());
  }

  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId " peer=%" PRI_FaceId " matching_suffix=%d", npe->name()->ToUri().c_str(), inface->id(), peer->id(), matching_suffix);
  
  npe->UpdateBestFace(peer->id());
}

void SelfLearnStrategy::FinalizeNpeExtra(void* extra) {
  if (extra == nullptr) return;
  
}

};//namespace ndnfd
