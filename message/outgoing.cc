#include "outgoing.h"
namespace ndnfd {

Outgoing::Outgoing(Ptr<Message> message, FaceId outgoing_face) {
  assert(message != nullptr);
  this->message_ = message;
  this->outgoing_face_ = outgoing_face;
  this->cancelled_ = false;
  this->sent_ = false;
  this->lock_.clear(std::memory_order_relaxed);
}

bool Outgoing::Cancel(void) {
  while (this->lock_.test_and_set(std::memory_order_acquire)) continue;
  bool ok = !this->sent_;
  this->cancelled_ = true;
  this->lock_.clear(std::memory_order_release);
  return ok;
}

bool Outgoing::Sending(void) {
  while (this->lock_.test_and_set(std::memory_order_acquire)) continue;
  bool ok = !this->cancelled_ && !this->sent_;
  this->sent_ = true;
  this->lock_.clear(std::memory_order_release);
  return ok;
}

};//namespace ndnfd
