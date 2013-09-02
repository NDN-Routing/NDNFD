#ifndef NDNFD_STRATEGY_ASL_H_
#define NDNFD_STRATEGY_ASL_H_
#include "nacks.h"
namespace ndnfd {

// AslStrategy is the adaptive self-learning forwarding strategy.
class AslStrategy : public NacksStrategy {
 public:
  AslStrategy(void);
  virtual ~AslStrategy(void) {}
  StrategyType_decl(AslStrategy);

  virtual void DidSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams);
  virtual void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent);

 protected:
  // PFI_FLOOD flag is set on upstream pit_face_item to indicate this upstream is created by Flood method.
  static const unsigned PFI_FLOOD = 0x40000;

  virtual std::unordered_set<FaceId> LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest);
  virtual Ptr<NamePrefixEntry> GetUpstreamStatusNode(Ptr<NamePrefixEntry> npe) { return npe; }
  // ForeachNpeAncestor invokes f on npe and its several ancestors.
  void ForeachNpeAncestor(Ptr<NamePrefixEntry> npe, std::function<void(Ptr<NamePrefixEntry>)> f);

  virtual void Forward(Ptr<PitEntry> ie);
  virtual bool DoForward(Ptr<PitEntry> ie) { assert(false); }
  void Flood(Ptr<PitEntry> ie);
  virtual void SendNack(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, NackCode code);
  virtual void ProcessNack(Ptr<PitEntry> ie, Ptr<const NackMessage> nack);
  virtual void DidnotArriveOnFace(Ptr<PitEntry> ie, FaceId face);
  virtual void OnRetryTimerExpire(Ptr<PitEntry> ie);
  
 private:
  static constexpr double PROBE_PROB = 0.1;
  std::default_random_engine probe_rand_;
  std::uniform_real_distribution<> probe_dist_;
  
  DISALLOW_COPY_AND_ASSIGN(AslStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_ASL_H
