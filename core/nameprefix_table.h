#ifndef NDNFD_CORE_NAMEPREFIX_TABLE_H_
#define NDNFD_CORE_NAMEPREFIX_TABLE_H_
#include "core/element.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
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
  
  // DeletePit consumes (deletes) a PIT entry.
  // Any reference to the PIT entry is invalidated.
  void DeletePit(Ptr<PitEntry> ie);
  
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
  
  // EnsureUpdatedFib ensures npe->forwarding and npe->tap are updated.
  void EnsureUpdatedFib(void) const;
  
  // LookupFib returns a set of outbound faces an Interest should be forwarded to.
  std::unordered_set<FaceId> LookupFib(Ptr<const InterestMessage> interest) const;
  
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
  enum class ForeachAction {
    kNone   = 0,
    kBreak  = 1,// stop iterating
    kDelete = 2,// delete current record
    kBreakDelete = kBreak | kDelete
  };
  
  PitEntry(Ptr<const Name> name, interest_entry* ie);
  virtual ~PitEntry(void) {}
  
  Ptr<const Name> name(void) const { return this->name_; }
  interest_entry* ie(void) const { return this->ie_; }
  
  // related NamePrefixEntry
  Ptr<NamePrefixEntry> npe(void) const;
  
  // SeekUpstream returns the upstream record for face.
  // If it does not exist, one is created.
  pit_face_item* SeekUpstream(FaceId face);
  
  void ForeachUpstream(std::function<ForeachAction(pit_face_item*)> f) { this->ForeachInternal(f, CCND_PFI_UPSTREAM); }

  // SeekDownstream returns the downstream record for face.
  // If it does not exist, one is created.
  pit_face_item* SeekDownstream(FaceId face);
  
  void ForeachDownstream(std::function<ForeachAction(pit_face_item*)> f) { this->ForeachInternal(f, CCND_PFI_DNSTREAM); }
  
  // NextEventDelay returns the delay until next pfi expires.
  // If include_expired is true, all pfi are considered;
  // otherwise, only pending downstream and unexpired upstream are considered.
  std::chrono::microseconds NextEventDelay(bool include_expired) const;

 private:
  Ptr<const Name> name_;
  interest_entry* ie_;
  
  void ForeachInternal(std::function<ForeachAction(pit_face_item*)> f, unsigned flag);

  DISALLOW_COPY_AND_ASSIGN(PitEntry);
};


};//namespace ndnfd
#endif//NDNFD_CORE_NAMEPREFIX_TABLE_H_
