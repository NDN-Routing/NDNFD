#include "util/buffer.h"
#include <cstring>
namespace ndnfd {

Buffer::Buffer(size_t size, size_t headroom, size_t tailroom) {
  this->initial_headroom_ = headroom;
  this->initial_tailroom_ = tailroom;
  this->headroom_ = headroom;
  this->c_ = ::ccn_charbuf_create_n(headroom + size + tailroom);
  this->c_->length = headroom + size;
}

Buffer::~Buffer(void) {
  ccn_charbuf_destroy(&(this->c_));
}

uint8_t* Buffer::Put(size_t n) {
  uint8_t* p = ::ccn_charbuf_reserve(this->c_, n);
  this->c_->length += n;
  return p;
}

void Buffer::Take(size_t n) {
  assert(n <= this->length());
  this->c_->length -= n;
}

uint8_t* Buffer::Push(size_t n) {
  if (n > this->headroom_) {
    size_t add_headroom = this->initial_headroom_ - this->headroom_ + n;
    this->headroom_ += add_headroom;
    ::ccn_charbuf_reserve(this->c_, add_headroom);
    ::memmove(this->c_->buf + add_headroom, this->c_->buf, this->c_->length);
    this->c_->length += add_headroom;
  }
  this->headroom_ -= n;
  return this->mutable_data();
}

void Buffer::Pull(size_t n) {
  assert(n <= this->length());
  this->headroom_ += n;
}

Ptr<Buffer> Buffer::AsBuffer(bool clone) {
  if (clone) {
    Buffer* other = new Buffer(this->length(), this->initial_headroom_, this->initial_tailroom_);
    ::memcpy(other->mutable_data(), this->mutable_data(), this->length());
    return other;
  } else {
    return this;
  }
}



Ptr<Buffer> BufferView::AsBuffer(bool clone) {
  //TODO implement
  return nullptr;
}

};//namespace ndnfd
