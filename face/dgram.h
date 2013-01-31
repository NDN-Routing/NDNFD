#ifndef NDNFD_FACE_DGRAM_H_
#define NDNFD_FACE_DGRAM_H_
#include "face/face.h"
namespace ndnfd {

class DgramMasterFace;

//communicate with one remote peer
class DgramFace : public Face {
  public:
    DgramFace(Ptr<DgramMasterFace> master, const NetworkAddress& peer);

    //encode, and pass to master.SlaveSend
    virtual void Send(Ptr<Message> message);
    
    //called by DgramMasterFace on receiving a packet from the same peer
    //pass into decoder.Input
    void MasterDeliver(const NetworkAddress& sender, Ptr<Buffer> pkt);
    
  private:
    NetwordAddress peer_;
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
//  (auto-create and deliver on the new face is not good, because there's no receiver on the new face)
class DgramMasterFace : public DgramFace {
  public:
    DgramMasterFace(Ptr<DgramChannel> channel, Ptr<FaceFactory> factory);

    virtual bool CanSend(void) const { return false; }
    virtual void Send(Ptr<Message> message) { assert(false); }
    
    //create a unicast face sharing the same channel
    virtual Ptr<DgramFace> MakeUnicast(const NetworkAddress& remote_addr);
    
    //called by DgramFace.Send
    //write to channel
    virtual void SlaveSend(const NetworkAddress& remote_addr, Ptr<Buffer> pkt);

  protected:
    Ptr<Channel> channel(void) const { return this->channel_; }
    Ptr<FaceFactory> factory(void) const { return this->factory_; }
  
  private:
    Ptr<Channel> channel_;
    Ptr<FaceFactory> factory_;//to verify address
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
