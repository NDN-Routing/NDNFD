#ifndef NDNFD_UTIL_BUFFER_H_
#define NDNFD_UTIL_BUFFER_H_
#include "util/defs.h"
extern "C" {
#include <ccn/charbuf.h>
}
namespace ndnfd {

class Buffer;

// A BufferView represents a read-only block of octets.
class BufferView : public Object {
 public:
  BufferView(Ptr<Buffer> buffer, size_t start, size_t length);
  virtual ~BufferView(void) {}
  
  // pointer to data
  virtual const uint8_t* data() const;
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
  virtual uint8_t* mutable_data() const;
  virtual const uint8_t* data() const { return this->mutable_data(); }
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
  
  // AsBuffer returns or clones self.
  virtual Ptr<Buffer> AsBuffer(bool clone) const;
  
 private:
  ::ccn_charbuf* c_;
  size_t headroom_;
  DISALLOW_COPY_AND_ASSIGN(Buffer);
};


inline const uint8_t* BufferView::data() const { return this->buffer_->data() + this->start_; }

};//namespace ndnfd
#endif//NDNFD_UTIL_BUFFER_H
