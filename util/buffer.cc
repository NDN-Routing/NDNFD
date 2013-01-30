#include "util/buffer.cc"
namespace ccnd2 {

Buffer* Buffer::Clone() {
  Buffer* copy = new Buffer(this->length());
  copy->Append(this);
  return copy;
}

void Buffer::CopyFrom(Buffer* source) {
  this->Reset();
  this->Append(source);
}

};//namespace ccnd2
