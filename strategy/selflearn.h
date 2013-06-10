#ifndef NDNFD_STRATEGY_SELFLEARN_H_
#define NDNFD_STRATEGY_SELFLEARN_H_
#include "strategy.h"
#include "util/rtt.h"
namespace ndnfd {

// SelfLearnStrategy is a prediction based self-learning strategy.
// Broadcast the first Interest. Learns about where are the fastest producer and other producers.
// Send subsequent Interests to the fastest producer, wait for predicted time,
// and then try other producers and also wait for each producer's predicted time.
// The predicted time is maintained by RttEstimator.
class SelfLearnStrategy : public Strategy {
 public:
  SelfLearnStrategy(void) {}
  virtual ~SelfLearnStrategy(void) {}

  virtual std::unordered_set<FaceId> LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest);
  virtual void PropagateNewInterest(Ptr<PitEntry> ie);
  virtual void DidnotArriveOnBestFace(Ptr<PitEntry> ie) { assert(false); }
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix);

  virtual void NewNpeExtra(Ptr<NamePrefixEntry> npe);
  virtual void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent);
  virtual void FinalizeNpeExtra(Ptr<NamePrefixEntry> npe);

 protected:
  // SendInterest sends Interest, and calls DidnotArriveOnFace if no ContentObject comes back in predict time.
  virtual void SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream);

 private:
  struct PredictRecord {
    std::chrono::microseconds time_;
    int rank_;
    std::chrono::microseconds accum_;
    RttEstimator rtt_;
  };
  struct NpeExtra {
    std::unordered_map<FaceId,PredictRecord> predicts_;
    std::unordered_map<FaceId,ccn_wrappedtime> interest_sent_time_;//per upstream: last time an Interest was sent
    void RankPredicts(void);
  };
  
  static std::chrono::microseconds initial_prediction(void) { return std::chrono::microseconds(15000); }
  // RankPredicts sorts the predicts set by increasing prediction.
  void RankPredicts(NpeExtra* extra);

  void DidnotArriveOnFace(Ptr<PitEntry> ie, FaceId face);
  
  // StartFlood adds new upstreams to broadcast the Interests
  void StartFlood(Ptr<PitEntry> ie);

  DISALLOW_COPY_AND_ASSIGN(SelfLearnStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_SELFLEARN_H
