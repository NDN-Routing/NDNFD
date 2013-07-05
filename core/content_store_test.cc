#include "content_store.h"
extern "C" {
#include <ccn/hashtb.h>
}
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

Ptr<InterestMessage> ContentStoreTest_MakeInterest(std::string name_uri, bool rightmost) {
  Ptr<Name> name = Name::FromUri(name_uri);
  std::basic_string<uint8_t> name_ccnb = name->ToCcnb();

  ccn_charbuf* c = ccn_charbuf_create();
  ccnb_element_begin(c, CCN_DTAG_Interest);
  ccn_charbuf_append(c, name_ccnb.data(), name_ccnb.size());
  if (rightmost) {
    ccnb_tagged_putf(c, CCN_DTAG_ChildSelector, "1");
  }
  ccnb_element_end(c);//</Interest>

  Ptr<InterestMessage> i = InterestMessage::Parse(c->buf, c->length);
  assert(i != nullptr);
  return i;
}

Ptr<ContentObjectMessage> ContentStoreTest_MakeCO(std::string name_uri) {
  Ptr<Name> name = Name::FromUri(name_uri);
  std::basic_string<uint8_t> name_ccnb = name->ToCcnb();

  ccn_charbuf* c = ccn_charbuf_create();
  ccnb_element_begin(c, CCN_DTAG_ContentObject);
  ccnb_element_begin(c, CCN_DTAG_Signature);
  ccnb_element_begin(c, CCN_DTAG_DigestAlgorithm);
  ccn_charbuf_append_tt(c, 3, CCN_UDATA);
  ccn_charbuf_append_string(c, "NOP");
  ccnb_element_end(c);//</DigestAlgorithm>
  ccnb_append_tagged_blob(c, CCN_DTAG_SignatureBits, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
  ccnb_element_end(c);//</Signature>
  ccn_charbuf_append(c, name_ccnb.data(), name_ccnb.size());
  ccnb_element_begin(c, CCN_DTAG_SignedInfo);
  ccnb_append_tagged_blob(c, CCN_DTAG_PublisherPublicKeyDigest, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
  ccnb_append_tagged_binary_number(c, CCN_DTAG_Timestamp, 0x10000);
  ccnb_element_end(c);//</SignedInfo>
  ccnb_append_tagged_blob(c, CCN_DTAG_Content, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
  ccnb_element_end(c);//</ContentObject>

  Ptr<ContentObjectMessage> co = ContentObjectMessage::Parse(c->buf, c->length);
  assert(co != nullptr);
  co = co->AddExplicitDigest();
  assert(co != nullptr);
  return co;
}

TEST(CoreTest, ContentStore) {
  ccnd_handle* h = ccnd_create("ndnfd", &Logging::CcndLogger, TestGlobal()->logging());
  TestGlobal()->set_ccndh(h);
  Ptr<ContentStore> cs = TestGlobal()->cs();

  EXPECT_EQ(0, cs->size());
  EXPECT_EQ(nullptr, cs->Get(1));

  auto co1 = ContentStoreTest_MakeCO("/ContentStoreTest/a/1/1");
  auto co2 = ContentStoreTest_MakeCO("/ContentStoreTest/a/aa/2");
  auto co3 = ContentStoreTest_MakeCO("/ContentStoreTest/a/aa/3");
  auto co4 = ContentStoreTest_MakeCO("/ContentStoreTest/b/4");
  
  ContentAccession a1, a2, a3, a4;
  std::unordered_set<ContentAccession> accessions;
  ContentStore::AddResult res; Ptr<ContentEntry> ce;

  std::tie(res, ce) = cs->Add(co1);
  EXPECT_EQ(ContentStore::AddResult::New, res);
  ASSERT_NE(nullptr, ce);
  accessions.insert(a1 = ce->accession());
  EXPECT_TRUE(ce->name()->Equals(co1->name()));
  
  std::tie(res, ce) = cs->Add(co2);
  EXPECT_EQ(ContentStore::AddResult::New, res);
  ASSERT_NE(nullptr, ce);
  accessions.insert(a2 = ce->accession());
  EXPECT_TRUE(ce->name()->Equals(co2->name()));
  
  std::tie(res, ce) = cs->Add(co3);
  EXPECT_EQ(ContentStore::AddResult::New, res);
  ASSERT_NE(nullptr, ce);
  accessions.insert(a3 = ce->accession());
  EXPECT_TRUE(ce->name()->Equals(co3->name()));
  
  std::tie(res, ce) = cs->Add(co4);
  EXPECT_EQ(ContentStore::AddResult::New, res);
  ASSERT_NE(nullptr, ce);
  accessions.insert(a4 = ce->accession());
  EXPECT_TRUE(ce->name()->Equals(co4->name()));
  
  std::tie(res, ce) = cs->Add(co4);
  EXPECT_EQ(ContentStore::AddResult::Duplicate, res);
  
  EXPECT_EQ(4, cs->size());
  EXPECT_EQ(4U, accessions.size());

  ce = cs->Get(a1);
  ASSERT_NE(nullptr, ce);
  EXPECT_EQ(a1, ce->accession());
  
  ce = cs->Lookup(ContentStoreTest_MakeInterest("/ContentStoreTest/a", false));
  ASSERT_NE(nullptr, ce);
  EXPECT_EQ(a1, ce->accession());

  ce = cs->Lookup(ContentStoreTest_MakeInterest("/ContentStoreTest/a", true));
  ASSERT_NE(nullptr, ce);
  EXPECT_EQ(a2, ce->accession());

  ce = cs->Lookup(ContentStoreTest_MakeInterest("/ContentStoreTest/c", false));
  EXPECT_EQ(nullptr, ce);

  hashtb_destroy(&(TestGlobal()->ccndh()->content_tab));
  hashtb_destroy(&(TestGlobal()->ccndh()->sparse_straggler_tab));
  TestGlobal()->ccndh()->content_tab = hashtb_create(sizeof(content_entry), nullptr);
  TestGlobal()->ccndh()->sparse_straggler_tab = hashtb_create(sizeof(sparse_straggler_entry), nullptr);
}

};//namespace ndnfd
