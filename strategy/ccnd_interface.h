#ifndef NDNFD_STRATEGY_CCND_INTERFACE_H_
#define NDNFD_STRATEGY_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

int propagate_interest(struct ccnd_handle* h, struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);
void strategy_callout2_SATISFIED(struct ccnd_handle* h, struct interest_entry* ie, struct face* from_face);
void note_content_from2(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned from_faceid, const uint8_t* name, size_t name_size, int matching_suffix);
void update_npe_children2(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid, const uint8_t* name, size_t name_size);
void finalize_nameprefix_strategy_extra(struct ccnd_handle* h, struct nameprefix_entry* npe);

#ifdef __cplusplus
}
#include "core/element.h"
#include "face/faceid.h"
#include "message/name.h"

namespace ndnfd {

class CcndStrategyInterface : public Element {
 public:
  CcndStrategyInterface(void) {}
  virtual ~CcndStrategyInterface(void) {}
  
  int PropagateInterest(face* face, uint8_t* msg, ccn_parsed_interest* pi, nameprefix_entry* npe);
  void WillSatisfyPendingInterest(interest_entry* ie, FaceId upstream);
  void DidSatisfyPendingInterests(nameprefix_entry* npe, FaceId upstream, Ptr<Name> name, int matching_suffix);
  void DidAddFibEntry(nameprefix_entry* npe, FaceId faceid, Ptr<Name> name);
  void FinalizeNpe(nameprefix_entry* npe);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(CcndStrategyInterface);
};


};//namespace ndnfd
#endif
#endif//NDNFD_STRATEGY_CCND_INTERFACE_H_
