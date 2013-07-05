#ifndef NDNFD_CORE_CONTENT_STORE_H_
#define NDNFD_CORE_CONTENT_STORE_H_
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "core/element.h"
#include "message/contentobject.h"
#include "message/interest.h"
namespace ndnfd {

typedef ccn_accession_t ContentAccession;

class ContentEntry;

// A ContentStore provides access to CS.
class ContentStore : public Element {
 public:
  enum class AddResult {
    New,      // new Content entry is created
    Refreshed,// stale Content entry is refreshed
    Duplicate,// ContentObject is a duplicate
    Collision,// key collision happens
    OverSize, // key is oversized
    Fail      // operation fails
  };
  
  ContentStore(void) {}
  virtual ~ContentStore(void) {}
  
  // number of entries
  int size(void) const;
  
  // Get retrieves a Content entry by its accession number.
  Ptr<ContentEntry> Get(ContentAccession accession);

  // Lookup finds the best matching Content entry for Interest.
  Ptr<ContentEntry> Lookup(Ptr<const InterestMessage> interest);
  
  // Add adds a ContentObject to CS.
  // co must have an explicit digest component.
  std::tuple<AddResult,Ptr<ContentEntry>> Add(Ptr<const ContentObjectMessage> co);
  
  // Remove deletes a ContentEntry.
  void Remove(Ptr<ContentEntry> ce);
  
 private:
  void CleanIfNecessary(void);
  ContentAccession NewAccession(void);
  
  DISALLOW_COPY_AND_ASSIGN(ContentStore);
};

// A ContentEntry is an entry in the ContentStore.
class ContentEntry : public Element {
 public:
  ContentEntry(content_entry* native, Ptr<const ContentObjectMessage> co);
  virtual ~ContentEntry(void) {}
  
  content_entry* native(void) const { return this->native_; }
  const uint8_t* msg(void) const { return static_cast<const uint8_t*>(this->native()->key); }
  size_t length(void) const { return this->native()->size; }
  Ptr<const Name> name(void) const;
  
  ContentAccession accession(void) const { return this->native()->accession; }
  bool stale(void) const { return (this->native()->flags & CCN_CONTENT_ENTRY_STALE) != 0; }
  const ccn_parsed_ContentObject* parsed(void) const { return &this->parsed_; }
  
  FaceId arrival_face(void) const { return static_cast<FaceId>(this->native()->arrival_faceid); }
  void set_arrival_face(FaceId value) { this->native()->arrival_faceid = static_cast<unsigned>(value); }
  
  bool unsolicited(void) const;
  void set_unsolicited(bool value);
  
  bool Enroll(void);
  
  // Refresh marks the message as non-stale, and sets the stale timer.
  void Refresh(void);
  
 private:
  content_entry* native_;
  ccn_parsed_ContentObject parsed_;
  
  DISALLOW_COPY_AND_ASSIGN(ContentEntry);
};

};//namespace ndnfd
#endif//NDNFD_CORE_CONTENT_STORE_H_
