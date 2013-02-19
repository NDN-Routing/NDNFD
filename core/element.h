#ifndef NDNFD_CORE_ELEMENT_H_
#define NDNFD_CORE_ELEMENT_H_
#include "core/global.h"
namespace ndnfd {

// Element base class.
class Element : public Object {
 public:
  // A PushPort is a function callback that passes information from this element to another.
  template<typename... TArgs>
  class PushPort {
   public:
    typedef std::function<void(TArgs&&...)> Callback;
    
    PushPort(void) { this->cb_ = nullptr; }
    
    // `port = callback` sets the callback.
    // This is typically used from an initialization function.
    void operator=(Callback value) { this->cb_ = value; }
    
    // `port(args)` invokes the callback.
    // This is typically used within the element defining the PushPort.
    void operator()(TArgs&... message) { if (this->cb() != nullptr) this->cb()(std::forward<TArgs...>(message...)); }
    void operator()(TArgs&&... message) { if (this->cb() != nullptr) this->cb()(std::forward<TArgs...>(message...)); }
    
    // callback function
    Callback cb() const { return this->cb_; }

   private:
    Callback cb_;
    DISALLOW_COPY_AND_ASSIGN(PushPort);
  };
  
  virtual ~Element(void) {}
  
  // MakeFirstElement creates a new element with a new Global object.
  static Ptr<Element> MakeFirstElement(Global* global);

  // New makes a new element of type T which shares the same Global object.
  template<typename T, typename... TArgs>
  Ptr<T> New(TArgs&&... args) const;

 protected:
  Element(void) { this->global_ = nullptr; }
  // Init is called after immediately constructor.
  // Element constructor does not have access to Global object,
  // but Init can access Global.
  virtual void Init(void) {}

  // the Global object
  Global* global() const { return const_cast<Global*>(this->global_); }
  
  void Log(LoggingLevel level, LoggingComponent component, const char* format, ...) const;

 private:
  Global* global_;
  DISALLOW_COPY_AND_ASSIGN(Element);
};

template<typename T, typename... TArgs>
Ptr<T> Element::New(TArgs&&... args) const {
  T* ele = new T(args...);
  ele->global_ = this->global_;
  static_cast<Element*>(ele)->Init();
  return ele;
}


};//namespace ndnfd
#endif//NDNFD_CORE_ELEMENT_H
