#ifndef NDNFD_CORE_NAMEPREFIX_TABLE_H_
#define NDNFD_CORE_NAMEPREFIX_TABLE_H_
extern "C" {
#include "ccnd/ccnd_private.h"
struct pit_face_item* pfi_set_nonce(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, const uint8_t* nonce, size_t noncesize);
int pfi_unique_nonce(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p);
void pfi_set_expiry_from_lifetime(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, intmax_t lifetime);
void pfi_set_expiry_from_micros(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, unsigned micros);
uint32_t WTHZ_value(void);
int wt_compare(ccn_wrappedtime a, ccn_wrappedtime b);
void adjust_npe_predicted_response(struct ccnd_handle* h, struct nameprefix_entry* npe, int up);
void adjust_predicted_response(struct ccnd_handle* h, struct interest_entry* ie, int up);
}
#include "core/element.h"
#include "util/foreach.h"
#include "message/interest.h"
namespace ndnfd {

class NamePrefixEntry;
class ForwardingEntry;
class PitEntry;
class PitUpstreamRecord;
class PitDownstreamRecord;

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

  FaceId best_faceid(void);
  void set_best_faceid(FaceId value) { this->npe()->src = value == FaceId_none ? CCN_NOFACEID : static_cast<unsigned>(value); }
  FaceId prev_best_faceid(void) const { return this->npe()->osrc == CCN_NOFACEID ? FaceId_none : static_cast<FaceId>(this->npe()->osrc); }
  
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
  
  // ForeachPit invokes f with each PitEntry whose name is under this prefix.
  void ForeachPit(std::function<ForeachAction(Ptr<PitEntry>)> f);
  
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
  FaceId face(void) const { return static_cast<FaceId>(this->forw()->faceid); }
  
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
  class PitFaceItem : public Element {
   public:
    PitFaceItem(Ptr<PitEntry> ie, pit_face_item* p);
    virtual ~PitFaceItem(void) {}
    Ptr<PitEntry> ie(void) const { return this->ie_; }
    FaceId faceid(void) const { return this->p_->faceid == CCN_NOFACEID ? FaceId_none : static_cast<FaceId>(this->p_->faceid); }
    
    pit_face_item* p(void) const { return this->p_; }
    void set_p(pit_face_item* value) { assert(value != nullptr); this->p_ = value; }
    
    virtual bool IsExpired(void) const =0;
    // CompareExpiry returns -1,0,1 if a expires earlier/same/later than b.
    static int CompareExpiry(Ptr<const PitFaceItem> a, Ptr<const PitFaceItem> b);

   protected:
    bool GetFlag(unsigned flag) const { return 0 != (this->p_->pfi_flags & flag); }
    void SetFlag(unsigned flag, bool value) { if (value) this->p_->pfi_flags |= flag; else this->p_->pfi_flags &= ~flag; }
    
   private:
    Ptr<PitEntry> ie_;
    pit_face_item* p_;
    DISALLOW_COPY_AND_ASSIGN(PitFaceItem);
  };
  
  template <typename TPfi>
  class PitFaceItemIterator : public std::iterator<std::forward_iterator_tag, Ptr<TPfi>> {
   public:
    explicit PitFaceItemIterator(Ptr<PitEntry> ie, pit_face_item* p) : ie_(ie) { assert(ie != nullptr); this->Update(p); }
    PitFaceItemIterator(const PitFaceItemIterator& other) : ie_(other.ie_), p_(other.p_), next_(other.next_) {}
    PitFaceItemIterator& operator++(void) { this->Update(this->next_); return *this; }
    PitFaceItemIterator operator++(int) { PitFaceItemIterator tmp(*this); this->operator++(); return tmp; }
    bool operator==(const PitFaceItemIterator& rhs) { return this->p_ == rhs.p_; }
    bool operator!=(const PitFaceItemIterator& rhs) { return this->p_ != rhs.p_; }
    Ptr<TPfi>& operator*(void) { assert(this->p_ != nullptr); return this->p_; }
    
    // Delete destroys the current record.
    // This iterator is still usable for advancing to the next record.
    void Delete(void);
    
   private:
    Ptr<PitEntry> ie_;
    Ptr<TPfi> p_;
    pit_face_item* next_;
    void Update(pit_face_item* p);
  };
  typedef PitFaceItemIterator<PitUpstreamRecord> UpstreamIterator;
  typedef PitFaceItemIterator<PitDownstreamRecord> DownstreamIterator;
  
  typedef uint32_t Serial;
#define PRI_PitEntrySerial PRIu32
  
  explicit PitEntry(interest_entry* ie);
  virtual ~PitEntry(void) {}
  
  Serial serial(void) const { return static_cast<Serial>(this->ie()->serial); }
  Ptr<const Name> name(void) const { return this->interest()->name(); }
  interest_entry* ie(void) const { return this->ie_; }
  
  // related NamePrefixEntry
  Ptr<NamePrefixEntry> npe(void) const;

  // related InterestMessage (without InterestLifetime and Nonce)
  Ptr<const InterestMessage> interest() const { return ie_ndnfdInterest(this->ie()); }
  
  // IsNonceUnique returns true if there's no other upstream/downstream record with same nonce as p.
  bool IsNonceUnique(Ptr<const PitFaceItem> p);
  
  // GetUpstream returns the upstream record for face, or null.
  Ptr<PitUpstreamRecord> GetUpstream(FaceId face) { return this->MakePfi<PitUpstreamRecord>(this->SeekPfiInternal(face, false, CCND_PFI_UPSTREAM)); }
  // SeekUpstream returns the upstream record for face.
  // If it does not exist, one is created.
  Ptr<PitUpstreamRecord> SeekUpstream(FaceId face) { return this->MakePfi<PitUpstreamRecord>(this->SeekPfiInternal(face, true, CCND_PFI_UPSTREAM)); }
  // beginUpstream and endUpstream iterates over upstream records.
  UpstreamIterator beginUpstream(void) { return UpstreamIterator(this, this->ie()->pfl); }
  UpstreamIterator endUpstream(void) { return UpstreamIterator(this, nullptr); }

  // GetDownstream returns the downstream record for face, or null.
  Ptr<PitDownstreamRecord> GetDownstream(FaceId face) { return this->MakePfi<PitDownstreamRecord>(this->SeekPfiInternal(face, false, CCND_PFI_DNSTREAM)); }
  // SeekDownstream returns the downstream record for face.
  // If it does not exist, one is created.
  Ptr<PitDownstreamRecord> SeekDownstream(FaceId face) { return this->MakePfi<PitDownstreamRecord>(this->SeekPfiInternal(face, true, CCND_PFI_DNSTREAM)); }
  // beginDownstream and endDownstream iterates over downstream records.
  DownstreamIterator beginDownstream(void) { return DownstreamIterator(this, this->ie()->pfl); }
  DownstreamIterator endDownstream(void) { return DownstreamIterator(this, nullptr); }
  
  // Delete pit_face_item.
  // Called by PitFaceItemIterator::Delete.
  void DeletePfiInternal(pit_face_item* p);
  
  // NextEventDelay returns the delay until next pfi expires.
  // If include_expired is true, all pfi are considered;
  // otherwise, only pending downstream and unexpired upstream are considered.
  std::chrono::microseconds NextEventDelay(bool include_expired) const;

 private:
  Ptr<const Name> name_;
  interest_entry* ie_;
  
  pit_face_item* SeekPfiInternal(FaceId face, bool create, unsigned flag);
  template <typename TPfi> Ptr<TPfi> MakePfi(pit_face_item* p) { if (p == nullptr) return nullptr; return this->New<TPfi>(this, p); }
  
  DISALLOW_COPY_AND_ASSIGN(PitEntry);
};

// A PitUpstreamRecord represents an upstream in PIT entry.
class PitUpstreamRecord : public PitEntry::PitFaceItem {
 public:
  static const unsigned ccnd_pfi_flag = CCND_PFI_UPSTREAM;

  PitUpstreamRecord(Ptr<PitEntry> ie, pit_face_item* p);
  virtual ~PitUpstreamRecord(void) {}
  
  bool pending(void) const { return this->GetFlag(CCND_PFI_UPENDING); }
  void set_pending(bool value) { this->SetFlag(CCND_PFI_UPENDING, value); }
  
  virtual bool IsExpired(void) const;
  
  void SetExpiry(std::chrono::microseconds t);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(PitUpstreamRecord);
};


// A PitDownstreamRecord represents a downstream in PIT entry.
class PitDownstreamRecord : public PitEntry::PitFaceItem {
 public:
  static const unsigned ccnd_pfi_flag = CCND_PFI_DNSTREAM;

  PitDownstreamRecord(Ptr<PitEntry> ie, pit_face_item* p);
  virtual ~PitDownstreamRecord(void) {}
  
  bool pending(void) const { return this->GetFlag(CCND_PFI_PENDING); }
  void set_pending(bool value) { this->SetFlag(CCND_PFI_PENDING, value); }
  
  bool suppress(void) const { return this->GetFlag(CCND_PFI_SUPDATA); }
  void set_suppress(bool value) { this->SetFlag(CCND_PFI_SUPDATA, value); }

  virtual bool IsExpired(void) const;
  
  // UpdateNonce sets pfi nonce to Interest's nonce, or generate a nonce.
  void UpdateNonce(Ptr<const InterestMessage> interest);
  
  // SetExpiryToLifetime sets pfi expiry time to InterestLifetime.
  void SetExpiryToLifetime(Ptr<const InterestMessage> interest);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(PitDownstreamRecord);
};


template <typename TPfi>
void PitEntry::PitFaceItemIterator<TPfi>::Delete(void) {
  assert(this->p_ != nullptr);
  this->ie_->DeletePfiInternal(this->p_->p());
  this->p_ = nullptr;
}

template <typename TPfi>
void PitEntry::PitFaceItemIterator<TPfi>::Update(pit_face_item* p) {
  while (p != nullptr && (p->pfi_flags & TPfi::ccnd_pfi_flag) == 0) {
    p = p->next;
  }
  if (p == nullptr) {
    this->p_ = nullptr;
    this->next_ = nullptr;
  }
  else {
    this->p_ = this->ie_->New<TPfi>(this->ie_, p);
    this->next_ = p->next;
  }
}

};//namespace ndnfd
#endif//NDNFD_CORE_NAMEPREFIX_TABLE_H_
