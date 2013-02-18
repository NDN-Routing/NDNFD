#include "element.h"
namespace ndnfd {

// TestGlobal is the Global object for Elements created by NewTestElement.
extern Global* TestGlobal;
Ptr<Element> TestFirstElement(void);

// NewTestElement creates an Element for test purpose.
template<typename T, typename... TArgs>
Ptr<T> NewTestElement(TArgs&&... args) {
  return TestFirstElement()->New<T>(args...);
}

};//namespace ndnfd
