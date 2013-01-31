#ifndef NDNFD_FACE_NDNLP_H_
#define NDNFD_FACE_NDNLP_H_
#include "face/dgram.h"
extern "C" {
#include "ndnld/ndnld.h"
}
namespace ndnfd {

class NdnlpFace : public DgramFace {
/*
NDNLP needs one set of {SequenceGenerator,SentPacketsStore,PartialMessageStore,AcknowledgeQueue} per peer.

To bridge with ndnld C code: this class acts like NdnlpSvc, but does not use CcnLAC or Link.
*/
  public:
    NdnlpFace(Ptr<NdnlpMasterFace> master, const NetworkAddress& peer);
    
    //if this face is auto-created for receiving, it's a dumb_receiver and cannot send
    //  in this state, only PartialMessageStore is working
    //  acknowledgements are accumulated in AcknowledgeQueue; they will be sent if this face is registered later
    //NdnlpMasterFace.MakeUnicast will register this face to be fully functional
    bool dumb_receiver(void) const { return this->id() == FaceId_none; }

    //similar to NdnlpSvc_ccn2link
    virtual void Send(Ptr<Message> message);

    //called by NdnlpMasterFace on receiving a packet from the same peer
    //similar to NdnlpSvc_link2ccn
    void MasterDeliver(const NetworkAddress& sender, Ptr<Buffer> pkt);
    
    //also need to schedule retransmit and acknowledge

  private:
    Ptr<NdnlpMasterFace> master_;
    
    DISALLOW_COPY_AND_ASSIGN(NdnlpFace);
}

class NdnlpMasterFace : public DgramMasterFace {
  public:
    NdnlpMasterFace(Ptr<DgramChannel> channel, Ptr<FaceFactory> factory);

    virtual bool CanSend(void) const { return false; }
    virtual void Send(Ptr<Message> message) { assert(false); }
    
    virtual Ptr<DgramFace> MakeUnicast(const NetworkAddress& remote_addr);

  private:
    Ptr<Channel> channel_;
    
    //connect to channel.Receive
    //demux, and call MasterDeliver of the unicast face (or self if there's no unicast face)
    void OnChannelReceive(const NetworkAddress& peer, Ptr<Buffer> pkt);
    
    DISALLOW_COPY_AND_ASSIGN(NdnlpMasterFace);
};

class NdnlpFaceFactory : Element {
  public:
    Ptr<NdnlpMasterFace> MakeMasterFace(const NetworkAddress& local_addr);
  private:
    DISALLOW_COPY_AND_ASSIGN(NdnlpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_NDNLP_H
