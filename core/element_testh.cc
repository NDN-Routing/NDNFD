#include "element_testh.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "core/pollmgr.h"
#include "core/scheduler.h"
#include "face/facemgr.h"
namespace ndnfd {

Global* theTestGlobal = nullptr;
Ptr<Element> theTestFirstElement = nullptr;

Global* TestGlobal(void) {
  if (theTestGlobal == nullptr) {
    theTestGlobal = new Global();
    theTestGlobal->Init();
  }
  return theTestGlobal;
}

Ptr<Element> TestFirstElement(void) {
  if (theTestFirstElement == nullptr) {
    theTestFirstElement = Element::MakeFirstElement(TestGlobal());
  }
  return theTestFirstElement;
}

};//namespace ndnfd
