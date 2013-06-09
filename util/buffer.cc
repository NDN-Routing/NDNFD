#include "util/buffer.h"
namespace ndnfd {

Buffer::Buffer(size_t length, size_t headroom, size_t tailroom) {
  this->initial_headroom_ = headroom;
  this->initial_tailroom_ = tailroom;
  this->headroom_ = headroom;
  this->c_ = ccn_charbuf_create_n(headroom + length + tailroom);
  this->c_->length = headroom + length;
}

Buffer::Buffer(uint8_t* data, size_t length) {
  this->initial_headroom_ = this->headroom_ = this->initial_tailroom_ = 0;
  this->c_ = ccn_charbuf_create();
  this->c_->buf = data;
  this->c_->length = length;
  this->c_->limit = length;
}

Buffer::Buffer(void) {
  this->initial_headroom_ = this->headroom_ = this->initial_tailroom_ = 0;
  this->c_ = nullptr;
}

Ptr<Buffer> Buffer::Adopt(ccn_charbuf** c) {
  Ptr<Buffer> b = new Buffer();
  b->c_ = *c;
  *c = nullptr;
  return b;
}

Buffer::~Buffer(void) {
  ccn_charbuf_destroy(&(this->c_));
}

uint8_t* Buffer::Reserve(size_t n) {
  return ccn_charbuf_reserve(this->c_, n);
}

uint8_t* Buffer::Put(size_t n) {
  uint8_t* p = this->Reserve(n);
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
    ccn_charbuf_reserve(this->c_, add_headroom);
    memmove(this->c_->buf + add_headroom, this->c_->buf, this->c_->length);
    this->c_->length += add_headroom;
  }
  this->headroom_ -= n;
  return this->mutable_data();
}

void Buffer::Pull(size_t n) {
  assert(n <= this->length());
  this->headroom_ += n;
}

void Buffer::Rebase(void) {
  if (this->headroom_ == this->initial_headroom_) return;

  size_t datalen = this->length();
  if (datalen == 0) {
    this->headroom_ = this->initial_headroom_;
    ccn_charbuf_reserve(this->c_, this->headroom_);
    this->c_->length = this->headroom_;
    return;
  }
  if (this->headroom_ < this->initial_headroom_) return;
  
  memmove(this->c_->buf + this->initial_headroom_, this->c_->buf + this->headroom_, datalen);
  this->headroom_ = this->initial_headroom_;
  this->c_->length = this->headroom_ + datalen;
}

Ptr<Buffer> Buffer::AsBuffer(bool clone) const {
  if (clone) {
    Buffer* other = new Buffer(this->length(), this->initial_headroom_, this->initial_tailroom_);
    memcpy(other->mutable_data(), this->mutable_data(), this->length());
    return other;
  } else {
    return const_cast<Buffer*>(this);
  }
}

std::tuple<uint8_t*,size_t> Buffer::Detach(void) {
  uint8_t* buf = this->c_->buf;
  size_t length = this->length();
  if (this->headroom_ != 0) {
    memmove(buf, buf + this->headroom_, length);
  }

  this->c_->buf = nullptr;
  this->c_->length = this->c_->limit = 0;
  this->initial_headroom_ = this->headroom_ = this->initial_tailroom_ = 0;

  return std::forward_as_tuple(buf, length);
}

void Buffer::Swap(ccn_charbuf* c) {
  uint8_t* buf; size_t length;
  std::tie(buf, length) = this->Detach();
  
  this->c_->buf = c->buf;
  this->c_->length = c->length;
  this->c_->limit = c->limit;
  
  c->buf = buf;
  c->length = c->limit = length;
}

BufferView::BufferView(Ptr<Buffer> buffer, size_t start, size_t length) {
  assert(buffer != nullptr);
  assert(buffer->length() >= start + length);
  this->buffer_ = buffer;
  this->start_ = start;
  this->length_ = length;
}

void BufferView::Take(size_t n) {
  assert(n <= this->length());
  this->length_ -= n;
}

void BufferView::Pull(size_t n) {
  assert(n <= this->length());
  this->start_ += n;
  this->length_ -= n;
}

Ptr<Buffer> BufferView::AsBuffer(bool clone) const {
  if (!clone && this->buffer_ != nullptr && this->start_ == 0U && this->length() == this->buffer_->length()) {
    return this->buffer_;
  }
  Ptr<Buffer> buffer = new Buffer(this->length());
  memcpy(buffer->mutable_data(), this->data(), this->length());
  return buffer;
}

};//namespace ndnfd
