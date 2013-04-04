# Object Model

NDNFD's object model is influenced by Click modular router and ns-3 network simulator. Using new features of C++ 11 standard, NDNFD's object model is much easier to implement than its predecessors.

## Object and Ptr

ndnfd::Object class provides reference counting, and automatic destruction.

`obj->Ref()` increments reference count by one.  
`obj->Unref()` decrements reference count by one; and deletes the object when reference count hits zero.

`Ptr<T>` is a smart pointer to type T. It can be used like a bare pointer `T*`.  
`Ptr<const T>` is the constant smart pointer can be used like `const T*`.  
Smart pointers can be constructed implicitly from bare pointers.

To obtain a regular pointer from a smart pointer, use:

	T* PeekPointer(Ptr<T> ptr); // return regular pointer without changing reference count
	T* GetPointer(PTr<T> ptr);  // increment reference count and return regular pointer

Type cast is a bit dirty:

	Ptr<SubClass> sub1 = new SubClass();
	Ptr<BaseClass> base1 = subobj;
	
	Ptr<BaseClass> base2 = new SubClass();
	Ptr<SubClass> sub2 = static_cast<SubClass*>(PeekPointer(base2));

A new object has initial reference count zero. Its owner should immediately assign it to a smart pointer, and keep a reference of the smart pointer. Otherwise, if the bare pointer is passed to some function that takes a smart pointer, the object will be destroyed when that function returns.

## Global

NDNFD cannot have any per-router state as global / static variable. Multiple NDNFD instances may be created within the same process (eg. in ns-3 simulation), which will cause conflicts in global variable access.

ndnfd::Global class keeps pointers to all global variables / objects used in an NDNFD instance. A pointer to the Global object is stored in every class that needs access to it.

## Element

ndnfd::Element class provides access to the Global object. Element is a subclass of Object. Any class that needs accessing the Global object should inherit from Element.

In an Element subclass, the Global object can be accessed with `this->global()`.

An Element subclass's constructor does not have access to the Global object. Accessing `this->global()` results in an assertion failure. An Element subclass can override `virtual void Init(void);` to complete initialization, which has access to the Global object.

To create an instance of an Element subclass, use the New method:

	Ptr<ElementTypeA> a = ...;
	Ptr<ElementTypeB> b = a->New<ElementTypeB>(ctor_args);

It's incorrect to create instances of an Element subclass using new operator, because that will not call the Init method or copy the Global pointer.

The *first* element is created with `Ptr<Element> Element::MakeFirstElement(Global* global)`, and then that element can be used to create other elements.

## PushPort

A ndnfd::Element::PushPort<T> represents an output port that an Element can pass messages of type T to another entity. A message passed into the PushPort is no longer owned by the previous Element; it is transferred to the receiving entity, or is destroyed if there is no receiving entity. The C++ template is actually declared as `template<typename... TArgs> class PushPort`, so that multiple objects can be passed together.

PushPort can be used like:

	class NetworkReceiver : public Element {
	 public:
	  PushPort<NetworkAddress,Ptr<Buffer>> Receive;
	  void SomeMethod(void) {
	    NetworkAddress addr = ...;
	    Ptr<Buffer> buf = ...;
	    this->Receive(addr, buf);
	  }
	};
	
	class SomeElement : public Element {
	 public:
	  void SomeInitializationMethod(void) {
	    this->nr_ = this->New<NetworkReceiver>();
	    this->nr_->Receive = std::bind(&SomeElement::OnNetworkReceive, this, std::placeholders::_1, std::placeholders::_2);
	  }
	 private:
	  Ptr<NetworkReceiver> nr_;
	  void OnNetworkReceive(NetworkAddress addr, Ptr<Buffer> buf);
	};


