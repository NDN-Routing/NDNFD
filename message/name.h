#ifndef NDNFD_MESSAGE_NAME_H_
#define NDNFD_MESSAGE_NAME_H_
#include "util/defs.h"
namespace ndnfd {

//an immutable Name
class Name {
  public:
    Name(const std::vector<std::string>& components) { this->components_ = components; }
    static Name FromCcnb(uint8_t* buf, size_t length);
    static Name FromUri(std::string uri);

    const std::vector<std::string>& components(void) const { return this->components_; }
    uint16_t n_components(void) const { return (uint16_t)this->components_.size(); }
    
    Name Append(std::string component) const;
    Name GetPrefix(uint16_t n_components) const;
    Name StripSuffix(uint16_t remove_n_components) const;
    
    void ToCcnb(ccn_charbuf* cb) const;
    std::string ToUri(void) const;
    bool IsPrefix(Name prefix) const;

  private:
    Name(void) {}
    std::vector<std::string> components_;
    DISALLOW_COPY_AND_ASSIGN(Name);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_NAME_H_
