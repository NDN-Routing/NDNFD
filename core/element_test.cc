#include "element.h"
#include "gtest/gtest.h"
namespace ndnfd {

class ElementTestElement : public Element {
  public:
    int a;
    int b;
    ElementTestElement(int a, int b) { this->a = a; this->b = b; }
    Global* TheGlobal(void) { return this->global(); }
  private:
    DISALLOW_COPY_AND_ASSIGN(ElementTestElement);
};

TEST(CoreTest, Element) {
  Global* global = new Global();
  Ptr<Element> first = Element::MakeFirstElement(global);
  Ptr<ElementTestElement> e1 = first->New<ElementTestElement>(1, 2);
  ASSERT_EQ(global, e1->TheGlobal());
  ASSERT_EQ(1, e1->a);
  ASSERT_EQ(2, e1->b);
}

};//namespace ndnfd
