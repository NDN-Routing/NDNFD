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
#include <boost/intrusive_ptr.hpp>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

namespace ndnfd {

// A Ptr is an intrusive smart pointer.
template<typename T>
using Ptr = boost::intrusive_ptr<T>;

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

void intrusive_ptr_add_ref(Object* p) {
  BOOST_ASSERT(p);
  p->Ref();
}
void intrusive_ptr_release(Object* p) {
  BOOST_ASSERT(p);
  p->Unref();
}

};//namespace ndnfd
#endif//NDNFD_CORE_DEFS_H
