#ifndef NDNFD_FACE_DGRAM_H_
#define NDNFD_FACE_DGRAM_H_
#include "face/face.h"
namespace ndnfd {

class DgramMasterFace;

//communicate with one remote peer
class DgramFace : public Face {
  public:
    DgramFace(Ptr<DgramMasterFace> master);

    virtual void Send(Ptr<Message> message);
    
    //called by DgramMasterFace on receiving a packet from the correct peer
    //pass into decoder.Input
    void MasterDeliver(Ptr<Buffer> pkt);
    
  private:
    Ptr<DgramMasterFace> master_;
    Ptr<Decoder> decoder_;
    
    //connect to decoder.Output
    //push into Receive
    void OnDecoderOutput(Ptr<Message> output);
    
    DISALLOW_COPY_AND_ASSIGN(DgramFace);
};

//communicate with many remote peers at the same local socket
//cannot send through this face because destination is unknown
//received packets are demuxed by sender address, and passed to unicast face of that sender
//  if the sender does not have a unicast face, the packet is received on this face
class DgramMasterFace : public DgramFace {
  public:
    DgramMasterFace(Ptr<Channel> channel);

    virtual bool CanSend(void) const { return false; }
    virtual void Send(Ptr<Message> message) { assert(false); }
    
    //create a unicast face sharing the same local socket
    Ptr<DgramFace> MakeUnicast(const NetworkAddress& remote_addr);

  private:
    Ptr<Channel> channel_;
    std::unordered_map<NetworkAddress,Ptr<DgramFace>> unicast_faces_;
    
    //connect to channel.Receive
    //demux, and call MasterDeliver of the unicast face (or self if there's no unicast face)
    void OnChannelReceive(const NetworkAddress& peer, Ptr<Buffer> pkt);
    
    DISALLOW_COPY_AND_ASSIGN(DgramMasterFace);
};

/*
//communicate on one multicast group
class MulticastFace : public DgramFace {
  public:
    MulticastFace(const NetworkAddress& local_addr, const NetworkAddress& mcast_group);

    virtual void Send(Ptr<Message> message);
    
  private:
    Ptr<Channel> channel_;
    DISALLOW_COPY_AND_ASSIGN(MasterFace);
};*/

};//namespace ndnfd
#endif//NDNFD_FACE_DGRAM_H
