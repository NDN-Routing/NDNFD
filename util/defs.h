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
#include <functional>

#include <boost/intrusive_ptr.hpp>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

namespace ccnd2 {

template<typename T>
using Ptr = boost::intrusive_ptr<T>;

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

class Global;

class Element : Object {
  public:
    template<typename TMessage>
    class PushPort {
      public:
        typedef std::function<void(TMessage)> Callback;
        void operator=(Callback value) { this->cb_ = value; }
        void operator()(Ptr<TMessage> message) { if (this->cb_ != NULL) this->cb_(message); }
      private:
        Callback cb_;
        DISALLOW_COPY_AND_ASSIGN(PushPort);
    };

  protected:
    Global* global();
    
    template<typename T, typename... TArgs>
    Ptr<T> Create(TArgs... args);

  private:
    Global* global_;
    DISALLOW_COPY_AND_ASSIGN(Element);
};


template<typename T, typename... TArgs>
Ptr<T> Element::Create(TArgs... args) {
  T* obj = new T(args...);
  if (obj != NULL) obj->global_ = this->global_;
  return obj;
}

};//namespace ccnd2
#endif//CCND2_UTIL_DEFS_H
