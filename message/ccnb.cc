#include "ccnb.h"
namespace ndnfd {

bool CcnbMessage::Verify(void) const {
  ::ccn_skeleton_decoder d;
  ::memset(&d, 0, sizeof(d));
  size_t dres = ::ccn_skeleton_decode(&d, this->msg(), this->length());
  return CCN_FINAL_DSTATE(d.state) && dres == this->length();
}

};//namespace ndnfd
