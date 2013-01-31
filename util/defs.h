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

//smart pointer
template<typename T>
using Ptr = boost::intrusive_ptr<T>;

//object base with smart pointer support
class Object {
  public:
    Object(void) { this->refcount_ = 0; }
    virtual ~Object(void) {}
    void Ref(void) const { ++this->refcount_; }
    void Unref(void) const { if (--this->refcount_ == 0) delete this; }

  private:
    std::atomic_size_t refcount_;
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
