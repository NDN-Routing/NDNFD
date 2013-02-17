#ifndef NDNFD_CORE_ELEMENT_H_
#define NDNFD_CORE_ELEMENT_H_
#include "core/global.h"
namespace ndnfd {

// An Element provides access to the Global object.
class Element : public Object {
 public:
  // A PushPort is a function callback that passes information from this element to another.
  template<typename... TArgs>
  class PushPort {
   public:
    typedef std::function<void(TArgs&...)> Callback;
    
    // `port = callback` sets the callback.
    // This is typically used from an initialization function.
    void operator=(Callback value) { this->cb_ = value; }
    
    // `port(args)` invokes the callback.
    // This is typically used within the element defining the PushPort.
    PushPort& operator()(TArgs&... message) { if (this->cb() != nullptr) this->cb()(message...); return *this; }
    
    // callback function
    Callback cb() const { return this->cb_; }

   private:
    Callback cb_;
    DISALLOW_COPY_AND_ASSIGN(PushPort);
  };
  
  virtual ~Element(void) {}
  
  // MakeFirstElement creates a new element with a new Global object.
  static Ptr<Element> MakeFirstElement(Global* global);

 protected:
  // the Global object
  Global* global() { return this->global_; }
  
  // Create makes a new element of type T which shares the same Global object.
  template<typename T, typename... TArgs>
  Ptr<T> Create(TArgs&&... args) const;

 private:
  Global* global_;
  DISALLOW_COPY_AND_ASSIGN(Element);
};

template<typename T, typename... TArgs>
Ptr<T> Element::Create(TArgs&&... args) const {
  T* obj = new T(args...);
  if (obj != NULL) obj->global_ = this->global_;
  return obj;
}


};//namespace ndnfd
#endif//NDNFD_CORE_ELEMENT_H
