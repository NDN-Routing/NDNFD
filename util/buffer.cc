#include "util/buffer.h"
namespace ndnfd {

CharBuf* CharBuf::Clone() {
  CharBuf* copy = new CharBuf(this->length());
  copy->Append(this);
  return copy;
}

void CharBuf::CopyFrom(CharBuf* source) {
  this->Reset();
  this->Append(source);
}

};//namespace ndnfd
