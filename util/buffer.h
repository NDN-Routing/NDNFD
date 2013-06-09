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

  // Take removes n octets at the end of data.
  virtual void Take(size_t n);
  
  // Pull removes n octets at the start of data.
  virtual void Pull(size_t n);
  
  // AsBuffer turns the BufferView into a Buffer.
  // If clone is true, the Buffer is guaranteed to have separate
  // storage with this BufferView.
  virtual Ptr<Buffer> AsBuffer(bool clone) const;

 protected:
  BufferView(void) { this->buffer_ = nullptr; this->start_ = this->length_ = 0; }
  
 private:
  Ptr<Buffer> buffer_;
  size_t start_;
  size_t length_;
  DISALLOW_COPY_AND_ASSIGN(BufferView);
};

// A Buffer represents a writable and resizable block of octets.
class Buffer : public BufferView {
 public:
  Buffer(size_t length, size_t headroom = 0, size_t tailroom = 0);
  Buffer(uint8_t* data, size_t length);
  virtual ~Buffer(void);
  // Adopt adopts *c, and c becomes invalid.
  static Ptr<Buffer> Adopt(ccn_charbuf** c);
  
  // pointer to data
  virtual uint8_t* mutable_data() const { return this->c_->buf + this->headroom_; }
  virtual const uint8_t* data() const { return this->mutable_data(); }
  // length of data
  virtual size_t length() const { return this->c_->length - this->headroom_; }
  
  // Reserve makes n octets space after the end of data,
  // and returns a pointer to the start of new space,
  // but does not increase length,
  uint8_t* Reserve(size_t n);

  // Put adds n octets after the end of data,
  // and returns a pointer to the start of new space.
  uint8_t* Put(size_t n);
  // Take removes n octets at the end of data.
  virtual void Take(size_t n);
  
  // Push adds n octets before the start of data,
  // and returns a pointer to the start of new space.
  uint8_t* Push(size_t n);
  // Pull removes n octets at the start of data.
  virtual void Pull(size_t n);

  // Rebase makes headroom <= initial_headroom by moving data.
  void Rebase(void);
  // Reset removes clears the buffer.
  void Reset(void) { this->Take(this->length()); }
  
  // AsBuffer returns or clones self.
  virtual Ptr<Buffer> AsBuffer(bool clone) const;
  // Detach detaches the octets from Buffer.
  // After this operation, this Buffer becomes empty.
  std::tuple<uint8_t*,size_t> Detach(void);
  // Swap swaps the contents of Buffer and c.
  void Swap(ccn_charbuf* c);
  
 private:
  Buffer(void);
  ccn_charbuf* c_;
  size_t initial_headroom_;
  size_t initial_tailroom_;
  size_t headroom_;
  DISALLOW_COPY_AND_ASSIGN(Buffer);
};


inline const uint8_t* BufferView::data() const { return this->buffer_->data() + this->start_; }

};//namespace ndnfd
#endif//NDNFD_UTIL_BUFFER_H
