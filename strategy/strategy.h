#ifndef NDNFD_STRATEGY_STRATEGY_H_
#define NDNFD_STRATEGY_STRATEGY_H_
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/face.h"
#include "message/interest.h"
#include "message/contentobject.h"
namespace ndnfd {

// A Strategy represents a forwarding strategy.
// This is not currently used.
// The design is taken from ndnSIM, but it seems insufficient.
class Strategy : public Element {
 public:
  Strategy(void);
  virtual ~Strategy(void);
  
  virtual void OnInterest(Ptr<InterestMessage> interest);
  
  virtual void OnContentObject(Ptr<ContentObjectMessage> content);
  
  virtual void WillEraseTimedOutPendingInterest(interest_entry* ie);
  
  virtual void AddFace(FaceId face);
  
  virtual void RemoveFace(FaceId face);
  
  virtual void DidAddFibEntry(nameprefix_entry* npe, ccn_forwarding* forw);
  
  virtual void WillRemoveFibEntry(nameprefix_entry* npe, ccn_forwarding* forw);
  
 protected:
  virtual void DoPropagateInterest(Ptr<InterestMessage> interest) =0;
 
 private:
  DISALLOW_COPY_AND_ASSIGN(Strategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_H
