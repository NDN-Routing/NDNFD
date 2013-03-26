#ifndef NDNFD_MESSAGE_NAME_H_
#define NDNFD_MESSAGE_NAME_H_
#include "util/defs.h"
extern "C" {
#include <ccn/charbuf.h>
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
  
  // ToCcnb writes Name as CCNB
  std::basic_string<uint8_t> ToCcnb(void) const;
  // ToUri writes Name as URI
  std::string ToUri(void) const;
  
  // Equals returns true if this and other are identical.
  bool Equals(const Ptr<Name> other) const;
  // IsPrefixOf returns true if this is a prefix of other
  bool IsPrefixOf(const Ptr<Name> other) const;

 private:
  Name(void) {}
  std::vector<Component> comps_;
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_NAME_H_
