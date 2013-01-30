#ifndef CCND2_UTIL_DEFS_H_
#define CCND2_UTIL_DEFS_H_

#include <assert.h>
#include <cstddef>
#include <cinttypes>
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <atomic>

#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)


#include "util/global.h"
namespace ccnd2 {

template<typename T>
using Ptr = boost::intrusive_ptr<T>;

class Object {
  public:
    virtual ~Object(void) {}
    void Ref(void) const { ++this->refcount_; }
    void Unref(void) const { if (--this->refcount_ == 0) delete this; }

  protected:
    Global* global();
    
    template<typename T, typename... TArgs>
    Ptr<T> Create(TArgs... args);
    
  private:
    std::atomic_size_t refcount_;
    Global* global_;
};

void intrusive_ptr_add_ref(Object* p) {
  BOOST_ASSERT(p);
  p->Ref();
}
void intrusive_ptr_release(Object* p) {
  BOOST_ASSERT(p);
  p->Unref();
}

template<typename T, typename... TArgs>
Ptr<T> Object::Create(TArgs... args) {
  T* obj = new T(args...);
  if (obj != NULL) obj->global_ = this->global_;
  return obj;
}

};//namespace ccnd2
#endif//CCND2_UTIL_DEFS_H
