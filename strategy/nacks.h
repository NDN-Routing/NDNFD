#ifndef NDNFD_STRATEGY_NACKS_H_
#define NDNFD_STRATEGY_NACKS_H_
#include "strategy.h"
namespace ndnfd {

// NacksStrategy is a basic NACK-enabled forwarding strategy.
class NacksStrategy : public Strategy {
 public:
  NacksStrategy(void) {}
  virtual ~NacksStrategy(void) {}
  StrategyType_decl(NacksStrategy);

  virtual void DidSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams);
  virtual void OnNack(Ptr<const NackMessage> nack);

  virtual void NewNpeExtra(Ptr<NamePrefixEntry> npe);
  virtual void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent);
  virtual void FinalizeNpeExtra(Ptr<NamePrefixEntry> npe);

 protected:
  // FaceStatus indicates the working status of each face regarding to data retrieval.
  // It is per name prefix, per face.
  enum class FaceStatus {
    kGreen  = 1,// the face can bring data back
    kYellow = 0,// it is unknown whether the face may bring data back
    kRed    = -1// the face cannot bring data back
  };
  static std::string FaceStatus_string(FaceStatus value);
  struct NpeExtra {
    std::unordered_map<FaceId,FaceStatus> status_;
  };

  // PFI_VAIN flag is set on pit_face_item when a Nack is received from that face.
  static const unsigned PFI_VAIN = 0x20000;
  // IE_RETRY_TIMER_EXPIRED flag is set on ccn_strategy.state when RetryTimer expires.
  static const int IE_RETRY_TIMER_EXPIRED = 0x1;
  
  // GetFaceStatusNode returns a NamePrefixEntry on which face status is stored.
  // NacksStrategy's impl returns npe->FibNode()
  virtual Ptr<NamePrefixEntry> GetFaceStatusNode(Ptr<NamePrefixEntry> npe);

  virtual void PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe);
  virtual void PropagateNewInterest(Ptr<PitEntry> ie) { assert(false); }
  
  virtual void Forward(Ptr<PitEntry> ie);
  // DoForward chooses one or more upstreams to forward the Interest.
  // It returns true if Interest is forwarded to at least one upstream,
  // or returns false if there's no usable upstream.
  virtual bool DoForward(Ptr<PitEntry> ie);
  
  // SetRetryTimer schedules or reschedules RetryTimer.
  void SetRetryTimer(Ptr<PitEntry> ie, std::chrono::microseconds delay);
  // IsRetryTimerExpired determines whether RetryTimer is expired.
  bool IsRetryTimerExpired(Ptr<PitEntry> ie);

 private:
  DISALLOW_COPY_AND_ASSIGN(NacksStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_NACKS_H
