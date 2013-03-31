#include "facemgr.h"
#include "netface.h"
namespace ndnfd {

void FaceMgrSim::StartDefaultListeners(void) {
  bool ok; NetworkAddress addr;
  std::tie(ok, addr) = EtherAddressVerifier::Parse("01:00:5E:00:17:AA"); assert(ok);
  Ptr<SimFaceFactory> ether_factory = this->New<SimFaceFactory>();
  char ifname[20];
  for (ns3::Ptr<ns3::NetDevice> dev : ether_factory->ListNICs()) {
    snprintf(ifname, sizeof(ifname), "%" PRIxMAX "", reinterpret_cast<uintmax_t>(ns3::PeekPointer(dev)));
    Ptr<DgramChannel> channel = ether_factory->Channel(dev, ns3::ndn::L3Protocol::ETHERNET_FRAME_TYPE);
    if (channel != nullptr) {
      Ptr<DgramFace> mcast_face = channel->GetMcastFace(addr);
      this->add_ether_channel(ifname, channel, mcast_face);
    }
  }
}

};//namespace ndnfd
