#ifndef NDNFD_UTIL_BUFFER_H_
#define NDNFD_UTIL_BUFFER_H_
#include "util/defs.h"
extern "C" {
#include <ccn/charbuf.h>
}
namespace ndnfd {

class CharBuf;

// A Buffer represents a resizable octet buffer.
class Buffer : public Object {
  public:
    Buffer(size_t size, size_t headroom = 0, size_t tailroom = 0);

    // pointer to data
    uint8_t* data();
    // pointer to end of data
    uint8_t* end_data() { return this->data() + this->length(); }
    // length of data
    size_t length();

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
    
  private:
    CharBuf cb_;
    size_t headroom_;
    DISALLOW_COPY_AND_ASSIGN(Buffer);
};

// don't use this directly; use Buffer instead
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
