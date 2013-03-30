#ifndef NDNFD_CORE_NAMEPREFIX_TABLE_H_
#define NDNFD_CORE_NAMEPREFIX_TABLE_H_
#include "core/element.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/faceid.h"
#include "message/interest.h"
namespace ndnfd {

class NamePrefixEntry;
class ForwardingEntry;
class PitEntry;

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
  
  // GetPit returns the PIT entry for interest,
  // or null if it does not exist.
  Ptr<PitEntry> GetPit(Ptr<const InterestMessage> interest);

  // SeekPit returns the PIT entry for interest,
  // or create it if it does not exist.
  Ptr<PitEntry> SeekPit(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe);
  
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
  
  // Parent returns the NamePrefixEntry for next shorter prefix.
  Ptr<NamePrefixEntry> Parent(void) const;
  
  // FibNode returns the current or parent NamePrefixEntry that has one or more forwarding entries.
  Ptr<NamePrefixEntry> FibNode(void) const;
  
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

// A PitEntry is a PIT entry, which represents one or more similar Interests
// and the associated propagation states.
class PitEntry : public Element {
 public:
  explicit PitEntry(interest_entry* ie);
  virtual ~PitEntry(void) {}
  
  Ptr<const Name> name(void) const;
  interest_entry* ie(void) const { return this->ie_; }
  
  // related NamePrefixEntry
  Ptr<NamePrefixEntry> npe(void) const;
  
  // Consume consumes (deletes) the PIT entry.
  // Any reference to this PitEntry is invalidated.
  void Consume(void);

 private:
  Ptr<const Name> name_;
  interest_entry* ie_;

  DISALLOW_COPY_AND_ASSIGN(PitEntry);
};

};//namespace ndnfd
#endif//NDNFD_CORE_NAMEPREFIX_TABLE_H_
