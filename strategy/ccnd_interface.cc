#include "ccnd_interface.h"
#include "strategy.h"
using ndnfd::Global;

int propagate_interest(struct ccnd_handle* h, struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe) {
  using ndnfd::Global;
  using ndnfd::Strategy;
  
  Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->PropagateInterest(face, msg, pi, npe);
}

namespace ndnfd {

int CcndStrategyInterface::PropagateInterest(struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe) {
  Ptr<InterestMessage> interest = new InterestMessage(msg, pi->offset[CCN_PI_E], pi);
  interest->set_incoming_face(static_cast<FaceId>(face->faceid));
  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(interest->name(), npe);
  this->global()->strategy()->PropagateInterest(interest, npe1);
  return 0;
}

};//namespace ndnfd

