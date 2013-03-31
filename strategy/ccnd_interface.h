#ifndef NDNFD_STRATEGY_CCND_INTERFACE_H_
#define NDNFD_STRATEGY_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

int propagate_interest(struct ccnd_handle* h, struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);

#ifdef __cplusplus
}
#include "core/element.h"

namespace ndnfd {

class CcndStrategyInterface : public Element {
 public:
  CcndStrategyInterface(void) {}
  virtual ~CcndStrategyInterface(void) {}
  
  int PropagateInterest(struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(CcndStrategyInterface);
};


};//namespace ndnfd
#endif
#endif//NDNFD_STRATEGY_CCND_INTERFACE_H_
