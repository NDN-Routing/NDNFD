#include "wireproto.h"
namespace ndnfd {

Ptr<Buffer> WireProtocolState::GetReceiveBuffer(void) {
  if (this->receive_buffer() == nullptr) {
    this->CreateReceiveBuffer();
  }
  return this->receive_buffer();
}

void WireProtocolState::CreateReceiveBuffer(void) {
  this->set_receive_buffer(new Buffer(0, 0, 1<<20));
}

};//namespace ndnfd
