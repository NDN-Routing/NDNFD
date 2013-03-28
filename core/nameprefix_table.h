#ifndef NDNFD_CORE_NAMEPREFIX_TABLE_H_
#define NDNFD_CORE_NAMEPREFIX_TABLE_H_
#include "core/element.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/faceid.h"
#include "message/name.h"
namespace ndnfd {

class NamePrefixEntry;
class ForwardingEntry;

// A NamePrefixTable provides access to PIT and FIB.
class NamePrefixTable : public Element {
 public:
  NamePrefixTable(void) {}
  virtual ~NamePrefixTable(void) {}
  
  // Get returns the nameprefix entry for name,
  // or null if it does not exist.
  Ptr<NamePrefixEntry> Get(Ptr<const Name> name) { return this->SeekInternal(name, false); }
  
  // Seek returns the nameprefix entry for name,
  // or create it, together with its parents, if it does not exist.
  Ptr<NamePrefixEntry> Seek(Ptr<const Name> name) { return this->SeekInternal(name, true); }
  
 private:
  Ptr<NamePrefixEntry> SeekInternal(Ptr<const Name> name, bool create);

  DISALLOW_COPY_AND_ASSIGN(NamePrefixTable);
};

// A NamePrefixEntry is an entry in the NamePrefixTable.
class NamePrefixEntry : public Element {
 public:
  NamePrefixEntry(Ptr<const Name> name, nameprefix_entry* npe);
  virtual ~NamePrefixEntry(void) {}
  
  Ptr<const Name> name(void) const { return this->name_; }
  nameprefix_entry* npe(void) const { return this->npe_; }
  
  // GetForwarding returns the forwarding entry for faceid,
  // or null if it does not exist.
  Ptr<ForwardingEntry> GetForwarding(FaceId faceid) { return this->SeekForwardingInternal(faceid, false); }

  // SeekForwarding returns the forwarding entry for faceid,
  // or create it if it does not exist.
  Ptr<ForwardingEntry> SeekForwarding(FaceId faceid) { return this->SeekForwardingInternal(faceid, true); }

 private:
  Ptr<const Name> name_;
  nameprefix_entry* npe_;

  Ptr<ForwardingEntry> SeekForwardingInternal(FaceId faceid, bool create);
  
  DISALLOW_COPY_AND_ASSIGN(NamePrefixEntry);
};

// A ForwardingEntry is a FIB entry.
class ForwardingEntry : public Element {
 public:
  ForwardingEntry(Ptr<NamePrefixEntry> npe, ccn_forwarding* forw);
  virtual ~ForwardingEntry(void) {}
  
  Ptr<const Name> name(void) const { return this->npe_->name(); }
  Ptr<NamePrefixEntry> npe(void) const { return this->npe_; }
  ccn_forwarding* forw(void) const { return this->forw_; }
  
  // Refresh makes the forwarding entry valid until now+expires,
  // or when face is closed.
  void Refresh(std::chrono::seconds expires);
  
  // MakePermanent makes the forwarding entry valid until the face is closed.
  void MakePermanent(void);

 private:
  Ptr<NamePrefixEntry> npe_;
  ccn_forwarding* forw_;
  
  DISALLOW_COPY_AND_ASSIGN(ForwardingEntry);
};

};//namespace ndnfd
#endif//NDNFD_CORE_NAMEPREFIX_TABLE_H_
