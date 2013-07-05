#include "content_store.h"
extern "C" {
#include <ccn/hashtb.h>
struct content_entry* content_from_accession(struct ccnd_handle* h, ccn_accession_t accession);
struct content_entry* find_first_match_candidate(struct ccnd_handle* h, const unsigned char* interest_msg, const struct ccn_parsed_interest* pi);
int content_matches_interest_prefix(struct ccnd_handle* h, struct content_entry* content, const unsigned char* interest_msg, struct ccn_indexbuf* comps, int prefix_comps);
struct content_entry* next_child_at_level(struct ccnd_handle* h, struct content_entry* content, int level);
ccn_accession_t content_skiplist_next(struct ccnd_handle* h, struct content_entry* content);
void enroll_content(struct ccnd_handle* h, struct content_entry* content);
void content_skiplist_insert(struct ccnd_handle* h, struct content_entry* content);
void clean_needed(struct ccnd_handle* h);
void set_content_timer(struct ccnd_handle* h, struct content_entry* content, struct ccn_parsed_ContentObject* pco);
int remove_content(struct ccnd_handle* h, struct content_entry* content);
}
extern "C" {
void ndnfd_finalize_ce(struct ccnd_handle* h, struct content_entry* entry) {
  ndnfd::ContentEntry* ce = static_cast<ndnfd::ContentEntry*>(entry->ndnfd_ce);
  ce->Unref();
}
}
#include "face/facemgr.h"
namespace ndnfd {

int ContentStore::size(void) const {
  return hashtb_n(CCNDH->content_tab);
}

Ptr<ContentEntry> ContentStore::Get(ContentAccession accession) {
  struct content_entry* native = content_from_accession(CCNDH, accession);
  if (native == nullptr) return nullptr;
  return static_cast<ContentEntry*>(native->ndnfd_ce);
}

Ptr<ContentEntry> ContentStore::Lookup(Ptr<InterestMessage> interest) {
  if ((interest->parsed()->answerfrom & CCN_AOK_CS) == 0) {
    // <AnswerOriginKind> doesn't permit ContentStore
    return nullptr;
  }
  bool stale_ok = (interest->parsed()->answerfrom & CCN_AOK_STALE) != 0;
  content_entry* last_match = nullptr;

  content_entry* content = find_first_match_candidate(CCNDH, interest->msg(), interest->parsed());
  while (content != nullptr) {
    // check if content is under Interest prefix
    if (!content_matches_interest_prefix(CCNDH, content, interest->msg(), const_cast<ccn_indexbuf*>(interest->comps()), interest->parsed()->prefix_comps)) {
      content = nullptr;
      break;
    }
    // check if matching
    Ptr<ContentEntry> ce = static_cast<ContentEntry*>(content->ndnfd_ce);
    bool advance_to_sibling = false;
    if ((stale_ok || (content->flags & CCN_CONTENT_ENTRY_STALE) == 0)
        && ccn_content_matches_interest(content->key, content->size, 0, const_cast<ccn_parsed_ContentObject*>(ce->parsed()), interest->msg(), interest->length(), interest->parsed())) {
      if ((interest->parsed()->orderpref && 1) == 0) {
        // <ChildSelector> prefers leftmost
        break;
      }
      // <ChildSelector> prefers rightmost
      last_match = content;
      advance_to_sibling = true;
    }
    // advance to next candidate
    if (advance_to_sibling) {
      content = next_child_at_level(CCNDH, content, interest->comps()->n - 1);
    } else {
      content = content_from_accession(CCNDH, content_skiplist_next(CCNDH, content));
    }
  }

  if (last_match != nullptr) content = last_match;
  if (content == nullptr) return nullptr;
  return static_cast<ContentEntry*>(content->ndnfd_ce);
}

std::tuple<ContentStore::AddResult,Ptr<ContentEntry>> ContentStore::Add(Ptr<const ContentObjectMessage> co) {
  assert(co->has_explicit_digest());
  struct hashtb_enumerator ee;
  struct hashtb_enumerator* e = &ee;
  hashtb_start(CCNDH->content_tab, e);
  
  size_t keysize = co->parsed()->offset[CCN_PCO_B_Content];
  if (keysize >= 65536) {
    hashtb_end(e);
    return std::forward_as_tuple(AddResult::OverSize, nullptr);
  }
  const uint8_t* tail = co->msg() + keysize;
  size_t tailsize = co->length() - keysize;
  int res = hashtb_seek(e, co->msg(), keysize, tailsize);
  content_entry* native = static_cast<content_entry*>(e->data);
  Ptr<ContentEntry> ce;

  if (res == HT_OLD_ENTRY) {
    ce = static_cast<ContentEntry*>(native->ndnfd_ce);
    if (tailsize != e->extsize || 0 != memcmp(tail, static_cast<const uint8_t*>(e->key) + keysize, tailsize)) {
      hashtb_delete(e);
      hashtb_end(e);
      return std::forward_as_tuple(AddResult::Collision, nullptr);
    }
    if (ce->stale()) {
      ce->Refresh();
      if (this->global()->facemgr()->GetFace(ce->arrival_face()) == nullptr) {
        ce->set_arrival_face(co->incoming_face());
      }
      hashtb_end(e);
      return std::forward_as_tuple(AddResult::Refreshed, ce);
    }
    ++(CCNDH->content_dups_recvd);
    hashtb_end(e);
    return std::forward_as_tuple(AddResult::Duplicate, ce);
  }
  
  this->CleanIfNecessary();
  native->accession = this->NewAccession();
  ce = this->New<ContentEntry>(native, co);
  native->ndnfd_ce = GetPointer(ce);
  native->key_size = e->keysize;
  native->size = e->keysize + e->extsize;
  native->key = static_cast<const unsigned char*>(e->key);
  if (!ce->Enroll()) {
    hashtb_delete(e);
    hashtb_end(e);
    return std::forward_as_tuple(AddResult::Fail, nullptr);
  }
  content_skiplist_insert(CCNDH, native);
  ce->Refresh();
  hashtb_end(e);
  return std::forward_as_tuple(AddResult::New, ce);
}

void ContentStore::CleanIfNecessary(void) {
  if (this->size() > static_cast<int>(CCNDH->capacity + (CCNDH->capacity >> 3))) {
    clean_needed(CCNDH);
  }
}

ContentAccession ContentStore::NewAccession(void) {
  return ++(CCNDH->accession);
}

void ContentStore::Remove(Ptr<ContentEntry> ce) {
  remove_content(CCNDH, ce->native());
}

ContentEntry::ContentEntry(content_entry* native, Ptr<const ContentObjectMessage> co) : native_(native) {
  memcpy(&this->parsed_, co->parsed(), sizeof(this->parsed_));
  this->set_arrival_face(co->incoming_face());
  const ccn_indexbuf* comps = co->comps();
  native->ncomps = comps->n;
  native->comps = static_cast<unsigned short*>(calloc(comps->n, sizeof(comps[0])));
  if (native->comps != nullptr) {
    for (size_t i = 0; i < comps->n; ++i) native->comps[i] = comps->buf[i];
  }
}

Ptr<const Name> ContentEntry::name(void) const {
  return Name::FromCcnb(this->msg() + this->parsed()->offset[CCN_PCO_B_Name], this->parsed()->offset[CCN_PCO_E_Name] - this->parsed()->offset[CCN_PCO_B_Name]);
}

bool ContentEntry::unsolicited(void) const {
  return ccn_indexbuf_member(CCNDH->unsol, this->accession()) >= 0;
}

void ContentEntry::set_unsolicited(bool value) {
  if (value) {
    this->native()->flags |= CCN_CONTENT_ENTRY_SLOWSEND;
    ccn_indexbuf_set_insert(CCNDH->unsol, this->accession());
  } else {
    this->native()->flags &= ~CCN_CONTENT_ENTRY_SLOWSEND;
    ccn_indexbuf_remove_element(CCNDH->unsol, this->accession());
  }
}

bool ContentEntry::Enroll(void) {
  if (this->native()->comps == nullptr) return false;
  enroll_content(CCNDH, this->native());
  return content_from_accession(CCNDH, this->accession()) == this->native();
}

// Refresh marks the message as non-stale, and sets the stale timer.
void ContentEntry::Refresh(void) {
  if (this->stale()) {
    this->native()->flags &= ~CCN_CONTENT_ENTRY_STALE;
    --(CCNDH->n_stale);
  }
  set_content_timer(CCNDH, this->native(), const_cast<ccn_parsed_ContentObject*>(this->parsed()));
}

};//namespace ndnfd
