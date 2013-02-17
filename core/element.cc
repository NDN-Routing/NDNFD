#include "element.h"
namespace ndnfd {

Ptr<Element> Element::MakeFirstElement(Global* global) {
  Ptr<Element> ele = new Element();
  ele->global_ = global;
  return ele;
}

};//namespace ndnfd
