#ifndef NDNFD_STRATEGY_CCND_INTERFACE_H_
#define NDNFD_STRATEGY_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

int propagate_interest(struct ccnd_handle* h, struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);
void update_npe_children2(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid, const uint8_t* name, size_t name_size);

#ifdef __cplusplus
}
#include "core/element.h"
#include "face/faceid.h"

namespace ndnfd {

class CcndStrategyInterface : public Element {
 public:
  CcndStrategyInterface(void) {}
  virtual ~CcndStrategyInterface(void) {}
  
  int PropagateInterest(struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);
  void DidAddFibEntry(struct nameprefix_entry* npe, FaceId faceid, const uint8_t* name, size_t name_size);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(CcndStrategyInterface);
};


};//namespace ndnfd
#endif
#endif//NDNFD_STRATEGY_CCND_INTERFACE_H_
