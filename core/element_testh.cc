#include "element_testh.h"
namespace ndnfd {

Global* TestGlobal = nullptr;
Ptr<Element> theTestFirstElement = nullptr;

Ptr<Element> TestFirstElement(void) {
  if (theTestFirstElement == nullptr) {
    TestGlobal = new Global();
    theTestFirstElement = Element::MakeFirstElement(TestGlobal);
  }
  return theTestFirstElement;
}

};//namespace ndnfd
