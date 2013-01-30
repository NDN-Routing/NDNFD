#ifndef NDNFD_CORE_ELEMENT_H_
#define NDNFD_CORE_ELEMENT_H_
#include "core/global.h"
namespace ndnfd {

class Element : Object {
  public:
    //a push port that can connect to another element
    template<typename TMessage>
    class PushPort {
      public:
        typedef std::function<void(TMessage)> Callback;
        void operator=(Callback value) { this->cb_ = value; }
        PushPort& operator<<(Ptr<TMessage> message) { if (this->cb_ != NULL) this->cb_(message); return *this; }
      private:
        Callback cb_;
        DISALLOW_COPY_AND_ASSIGN(PushPort);
    };
    //declare as: PushPort<Message> event
    //connect as: event = callback;
    //send as: event << message;
    
    static Ptr<Element> MakeFirstElement(Global* global);

  protected:
    Global* global() { return this->global_; }
    
    //create a new element, inheriting global object
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


};//namespace ndnfd
#endif//NDNFD_CORE_ELEMENT_H
