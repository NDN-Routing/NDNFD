#ifndef NDNFD_STRATEGY_ORIGINAL_H_
#define NDNFD_STRATEGY_ORIGINAL_H_
#include "strategy.h"
#include "core/scheduler.h"
namespace ndnfd {

// OriginalStrategy is an reimplementation of ccnd default strategy.
class OriginalStrategy : public Strategy {
 public:
  OriginalStrategy(void) {}
  virtual void Init2(void);
  virtual ~OriginalStrategy(void);

  virtual void PropagateNewInterest(Ptr<PitEntry> ie);
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix);
  virtual void DidAddFibEntry(Ptr<ForwardingEntry> forw);

  virtual void NewNpeExtra(Ptr<NamePrefixEntry> npe);
  virtual void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent);
  virtual void FinalizeNpeExtra(Ptr<NamePrefixEntry> npe);

 protected:
  struct NpeExtra {
    FaceId best_faceid_;
    FaceId prev_faceid_;
    std::chrono::microseconds prediction_;
    
    NpeExtra(void);
    // GetBestFace returns best_faceid.
    // If it's FaceId_none, it returns prev_faceid and writes that to best_faceid.
    FaceId GetBestFace(void);
    // UpdateBestFace updates best_faceid.
    // If best_faceid is same as value, it also adjusts down predicted response time.
    void UpdateBestFace(FaceId value);
    // AdjustPredictUp adjusts up predicted response time.
    void AdjustPredictUp(void);
    // AgeBestFace clears best face information.
    void AgeBestFace(void);
  };

  SchedulerEvent agebestface_evt_;
  std::chrono::microseconds AgeBestFace(void);

  // DidnotArriveOnBestFace is invoked when ContentObject didn't arrived on best face
  // in predicted time.
  // (same as ccnd strategy_callout CCNST_TIMER)
  virtual void DidnotArriveOnBestFace(Ptr<PitEntry> ie);

 private:
  DISALLOW_COPY_AND_ASSIGN(OriginalStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_ORIGINAL_H
