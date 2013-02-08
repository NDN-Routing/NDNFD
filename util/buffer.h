#ifndef NDNFD_UTIL_BUFFER_H_
#define NDNFD_UTIL_BUFFER_H_
#include "util/defs.h"
extern "C" {
#include <ccn/charbuf.h>
}
namespace ndnfd {

class CharBuf;

// A BufferView represents a read-only block of octets.
class BufferView : public Object {
 public:
  BufferView(Ptr<Buffer> buffer, size_t start, size_t length);
  virtual ~BufferView(void) {}
  
  // pointer to data
  virtual const uint8_t* data() const { return this->buffer_->data() + this->start_; }
  // pointer to end of data
  virtual const uint8_t* end_data() const { return this->data() + this->length(); }
  // length of data
  virtual size_t length(void) const { return this->length_; }
  
  // AsBuffer turns the BufferView into a Buffer.
  // If clone is true, the Buffer is guaranteed to have separate
  // storage with this BufferView.
  virtual Ptr<Buffer> AsBuffer(bool clone) const;
  
 private:
  Ptr<Buffer> buffer_;
  size_t start_;
  size_t length_;
  DISALLOW_COPY_AND_ASSIGN(BufferView);
};

// A Buffer represents a writable and resizable block of octets.
class Buffer : public BufferView {
 public:
  Buffer(size_t size, size_t headroom = 0, size_t tailroom = 0);
  virtual ~Buffer(void) {}

  // pointer to data
  virtual uint8_t* data() const;
  // pointer to end of data
  virtual uint8_t* end_data() const { return this->data() + this->length(); }
  // length of data
  virtual size_t length() const;

  // Put adds n octets after the end of data,
  // and returns a pointer to the start of new space.
  uint8_t* Put(size_t n);
  // Take removes n octets at the end of data.
  void Take(size_t n);
  
  // Push adds n octets before the start of data,
  // and returns a pointer to the start of new space.
  uint8_t* Push(size_t n);
  // Pull removes n octets at the start of data.
  void Pull(size_t n);
  
  // AsBuffer returns self.
  virtual Ptr<Buffer> AsBuffer(bool clone) const;
  
 private:
  CharBuf cb_;
  size_t headroom_;
  DISALLOW_COPY_AND_ASSIGN(Buffer);
};

// DEPRECATED: use Buffer instead
class CharBuf : public Object {
 public:
  CharBuf() { this->c_ = ::ccn_charbuf_create(); }
  CharBuf(size_t n) { this->c_ = ::ccn_charbuf_create_n(n); }
  ~CharBuf() { ::ccn_charbuf_destroy(&this->c_); }
  CharBuf* Clone();
  void CopyFrom(CharBuf* source);

  size_t length() const { return this->c_->length; }
  void set_length(size_t value) { this->c_->length = value; }
  size_t inc_length(size_t diff) { return this->c_->length += diff; }
  size_t limit() const { return this->c_->limit; }
  uint8_t* buf() const { return this->c_->buf; }
  uint8_t* Reserve(size_t n) { return ::ccn_charbuf_reserve(this->c_, n); }
  void Reset() { ::ccn_charbuf_reset(this->c_); }
  int Append(const void* p, size_t n) { return ::ccn_charbuf_append(this->c_, p, n); }
  int Append(CharBuf* i) { return ::ccn_charbuf_append_charbuf(this->c_, i->c_); }
  int Append(const char* s) { return ::ccn_charbuf_append_string(this->c_, s); }
  char* AsString() const { return ::ccn_charbuf_as_string(this->c_); }

  ::ccn_charbuf* c() { return this->c_; }
  
 private:
  ::ccn_charbuf* c_;
  DISALLOW_COPY_AND_ASSIGN(CharBuf);
};

};//namespace ndnfd
#endif//NDNFD_UTIL_BUFFER_H
