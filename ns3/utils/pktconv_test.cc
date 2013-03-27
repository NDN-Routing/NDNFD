#include "pktconv.h"
#include <ns3/packet.h>
#include "gtest/gtest.h"
namespace ndnfd {

TEST(SimTest, NdnsimPacketConverter) {
  Ptr<NdnsimPacketConverter> npc = new NdnsimPacketConverter();

  ns3::ndn::Name sn1;
  sn1("hello")("world");
  Ptr<Name> n1 = npc->NameFrom(sn1);
  EXPECT_TRUE(Name::FromUri("/hello/world")->Equals(n1));
  ns3::Ptr<ns3::ndn::Name> sn2 = npc->NameTo(n1);
  EXPECT_EQ(sn1, *sn2);

  ns3::ndn::Interest si1;
  si1.SetName(sn2);
  Ptr<InterestMessage> i1 = npc->InterestFrom(si1);
  ASSERT_NE(nullptr, i1);
  EXPECT_TRUE(n1->Equals(i1->name()));
  
  ns3::Ptr<ns3::Packet> si2p = npc->MessageTo(i1);
  ns3::ndn::Interest si2;
  si2p->RemoveHeader(si2);
  EXPECT_EQ(si1.GetName(), si2.GetName());
  
  ns3::ndn::ContentObject sco1;
  sco1.SetName(sn2);
  sco1.SetTimestamp(ns3::Seconds(16));
  ns3::Ptr<ns3::Packet> sco1payload = ns3::Create<ns3::Packet>(reinterpret_cast<const uint8_t*>("ABCD"), 4);
  Ptr<ContentObjectMessage> co1 = npc->ContentObjectFrom(sco1, sco1payload);
  ASSERT_NE(nullptr, co1);
  EXPECT_TRUE(n1->Equals(co1->name()));
  
  ns3::Ptr<ns3::Packet> sco2p = npc->MessageTo(co1);
  ASSERT_NE(nullptr, sco2p);
  Ptr<CcnbMessage> co2m = npc->MessageFrom(sco2p);
  ASSERT_NE(nullptr, co2m);
  ASSERT_EQ(ContentObjectMessage::kType, co2m->type());
  Ptr<ContentObjectMessage> co2 = static_cast<ContentObjectMessage*>(PeekPointer(co2m));
  EXPECT_TRUE(n1->Equals(co2->name()));
  const uint8_t* co2payload; size_t co2payload_size;
  std::tie(co2payload, co2payload_size) = co2->payload();
  ASSERT_EQ(4U, co2payload_size);
  EXPECT_EQ(static_cast<uint8_t>('D'), co2payload[3]);
}

};//namespace ndnfd
