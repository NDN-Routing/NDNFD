#ifndef NDNFD_STRATEGY_CCND_INTERFACE_H_
#define NDNFD_STRATEGY_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

void strategy_callout2_SATISFIED(struct ccnd_handle* h, struct interest_entry* ie, int pending_downstreams);
void note_content_from2(struct ccnd_handle* h, struct nameprefix_entry* npe, int matching_suffix);

#ifdef __cplusplus
}
#include "core/content_store.h"
#include "message/contentobject.h"

namespace ndnfd {

class CcndStrategyInterface : public Element {
 public:
  // current processing ContentEntry and ContentObject
  Ptr<const ContentEntry> ce_;
  Ptr<const ContentObjectMessage> co_;
  
  CcndStrategyInterface(void) {}
  virtual ~CcndStrategyInterface(void) {}
  
  void DidSatisfyPendingInterest(interest_entry* ie, int pending_downstreams);
  void DidReceiveContent(nameprefix_entry* npe, int matching_suffix);
  void DidAddFibEntry(nameprefix_entry* npe, FaceId faceid);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(CcndStrategyInterface);
};


};//namespace ndnfd
#endif
#endif//NDNFD_STRATEGY_CCND_INTERFACE_H_
