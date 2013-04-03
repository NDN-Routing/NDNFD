#include "selflearn.h"
#include <algorithm>
#include "face/facemgr.h"
namespace ndnfd {

std::unordered_set<FaceId> SelfLearnStrategy::LookupOutbounds(Ptr<NamePrefixEntry> npe, Ptr<InterestMessage> interest, Ptr<PitEntry> ie) {
  // collect downstreams
  std::vector<Ptr<Face>> downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    Ptr<Face> downstream = this->global()->facemgr()->GetFace(p->faceid());
    if (downstream != nullptr) downstreams.push_back(downstream);
  });

  // lookup FIB
  std::unordered_set<FaceId> fib_outbounds = npe->LookupFib(interest);
  std::unordered_set<FaceId> outbounds;

  // select outbounds
  for (FaceId outbound : fib_outbounds) {
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

bool SelfLearnStrategy::DidExhaustForwardingOptions(Ptr<PitEntry> ie) {
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
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::DidExhaustForwardingOptions(%" PRI_PitEntrySerial ") broadcast=[%s]", ie->serial(), debug_list.c_str());

  this->PopulateOutbounds(ie, outbounds);
  return !outbounds.empty();
}

void SelfLearnStrategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, FaceId upstream) {
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId "", npe->name()->ToUri().c_str(), upstream);
  int limit = 2;//TODO consider increase this value
  for (; npe != nullptr && --limit >= 0; npe = npe->Parent()) {
    if (npe->npe()->src == static_cast<unsigned>(upstream)) {
      adjust_npe_predicted_response(this->global()->ccndh(), npe->npe(), 0);
      continue;
    }
    if (npe->npe()->src != CCN_NOFACEID) {
      npe->npe()->osrc = npe->npe()->src;
    }
    npe->npe()->src = static_cast<unsigned>(upstream);
  }
}

};//namespace ndnfd
