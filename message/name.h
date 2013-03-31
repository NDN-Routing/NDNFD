#ifndef NDNFD_MESSAGE_NAME_H_
#define NDNFD_MESSAGE_NAME_H_
#include "util/defs.h"
extern "C" {
#include <ccn/charbuf.h>
#include <ccn/indexbuf.h>
}
namespace ndnfd {

// A Name represents an NDN Name.
// Name is immutable; every operation returns a new Name.
// This is not currently used.
class Name : public Object {
 public:
  typedef std::basic_string<uint8_t> Component;
  
  // Name creates Name from components.
  explicit Name(const std::vector<Component>& comps) { this->comps_ = comps; }
  // FromCcnb parses Name from CCNB.
  // CCNB could be either <Name><Component>...</Component><Component>...</Component></Name>,
  // or <Component>...</Component><Component>...</Component>.
  static Ptr<Name> FromCcnb(const uint8_t* buf, size_t length);
  // FromUri parses Name from URI.
  static Ptr<Name> FromUri(const std::string& uri);

  // components
  const std::vector<Component>& comps(void) const { return this->comps_; }
  // number of components
  uint16_t n_comps(void) const { return static_cast<uint16_t>(this->comps_.size()); }
  
  // Append returns a new Name that is this+component
  Ptr<Name> Append(const Component& component) const;
  // GetPrefix returns a new Name that has the first_n components of this
  Ptr<Name> GetPrefix(uint16_t first_n) const;
  // StripSuffix returns a new Name that has the first (this->n_comps() - remove_n) components of this
  Ptr<Name> StripSuffix(uint16_t remove_n) const;
  
  // ToCcnb returns CCNB for Name.
  // If tag_Name is true, <Name>..</Name> tags are included; otherwise they are not included.
  // If comps is not null, it is populated with positions of <Component> elements and beyond last </Component>.
  std::basic_string<uint8_t> ToCcnb(bool tag_Name = true, ccn_indexbuf* comps = nullptr) const;
  // ToUri returns URI for Name.
  std::string ToUri(void) const;
  
  // Equals returns true if this and other are identical.
  bool Equals(Ptr<const Name> other) const;
  // IsPrefixOf returns true if this is a prefix of other
  bool IsPrefixOf(Ptr<const Name> other) const;

 private:
  Name(void) {}
  std::vector<Component> comps_;
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_NAME_H_
