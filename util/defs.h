#ifndef NDNFD_CORE_DEFS_H_
#define NDNFD_CORE_DEFS_H_

#include <assert.h>
#include <cstddef>
#include <cinttypes>
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <functional>
#include "ptr.h"

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

namespace ndnfd {

// A Ptr is an intrusive smart pointer.
template<typename T>
using Ptr = ns3::Ptr<T>;

// Object is the based class for objects that needs smart pointer support.
class Object {
 public:
  Object(void) { this->refcount_ = 0; }
  virtual ~Object(void) {}
  
  // Ref increments the reference counter.
  void Ref(void) const { ++(const_cast<Object*>(this)->refcount_); }
  
  // Unref decrements the reference counter,
  // and deletes the object if no longer needed.
  void Unref(void) const { if (--(const_cast<Object*>(this)->refcount_) == 0) delete this; }

 private:
  std::atomic_size_t refcount_;
  DISALLOW_COPY_AND_ASSIGN(Object);
};

};//namespace ndnfd
#endif//NDNFD_CORE_DEFS_H
