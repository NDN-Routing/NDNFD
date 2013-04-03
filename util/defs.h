#ifndef NDNFD_UTIL_DEFS_H_
#define NDNFD_UTIL_DEFS_H_

#include <assert.h>
#include <cstddef>
#include <cstdlib>
#include <cinttypes>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <tuple>
#include <map>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <chrono>
#include "ptr.h"

namespace std {
template <typename T> struct hash<ndnfd::Ptr<T>> {
  size_t operator()(ndnfd::Ptr<T> ptr) const { std::hash<T*> hasher; return hasher(ndnfd::PeekPointer(ptr)); }
};
};

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

namespace ndnfd {

// A Ptr is an intrusive smart pointer.

// Object is the based class for objects that needs smart pointer support.
class Object {
 public:
  Object(void) : finalized_(ATOMIC_FLAG_INIT) { this->refcount_ = 0; }
  virtual ~Object(void) {}
  
  // Ref increments the reference counter.
  void Ref(void) const { ++(const_cast<Object*>(this)->refcount_); }
  
  // Unref decrements the reference counter,
  // and deletes the object if no longer needed.
  void Unref(void) const {
    if (--(const_cast<Object*>(this)->refcount_) == 0) {
      if (!const_cast<Object*>(this)->finalized_.test_and_set()) delete this;
    }
  }

 private:
  std::atomic_flag finalized_;
  std::atomic_size_t refcount_;
  DISALLOW_COPY_AND_ASSIGN(Object);
};

};//namespace ndnfd
#endif//NDNFD_UTIL_DEFS_H
