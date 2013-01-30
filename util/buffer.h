#ifndef CCND2_UTIL_BUFFER_H_
#define CCND2_UTIL_BUFFER_H_
#include "util/defs.h"
extern "C" {
#include <ccn/charbuf.h>
}
namespace ccnd2 {

class Buffer {
  public:
    Buffer() { this->c_ = ::ccn_charbuf_create(); }
    Buffer(size_t n) { this->c_ = ::ccn_charbuf_create_n(n); }
    ~Buffer() { ::ccn_charbuf_destroy(&this->c_); }
    Buffer* Clone();
    void CopyFrom(Buffer* source);

    size_t length() const { return this->c_->length; }
    void set_length(size_t value) { this->c_->length = value; }
    size_t inc_length(size_t diff) { return this->c_->length += diff; }
    size_t limit() const { return this->c_->limit; }
    uint8_t* buf() const { return this->c_->buf; }
    uint8_t* Reserve(size_t n) { return ::ccn_charbuf_reserve(this->c_, n); }
    void Reset() { ::ccn_charbuf_reset(this->c_); }
    int Append(const void* p, size_t n) { return ::ccn_charbuf_append(this->c_, p, n); }
    int Append(Buffer* i) { return ::ccn_charbuf_append_charbuf(this->c_, i->c_); }
    int Append(const char* s) { return ::ccn_charbuf_append_string(this->c_, s); }
    char* AsString() const { return ::ccn_charbuf_as_string(this->c_); }
    
    ::ccn_charbuf* c() { return this->c_; }
    
  private:
    ::ccn_charbuf* c_;
    DISALLOW_COPY_AND_ASSIGN(Buffer);
};

};//namespace ccnd2
#endif//CCND2_UTIL_BUFFER_H
