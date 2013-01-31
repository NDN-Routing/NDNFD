#ifndef NDNFD_FACE_STREAM_H_
#define NDNFD_FACE_STREAM_H_
#include "face/face.h"
namespace ndnfd {

class StreamFace : public Face {
  public:
    StreamFace(Ptr<Channel> channel);
    virtual void Send(Ptr<Message> message);

  private:
    //decoder provides ReceiveBuffer to channel, avoid copying
    Ptr<Channel> channel_;
    Ptr<Decoder> decoder_;

    //connect to channel.Receive
    //pass into decoder.Input
    void OnChannelReceive(const NetworkAddress& peer, Ptr<Buffer> pkt);
    //connect to decoder.Output
    //push into Receive
    void OnDecoderOutput(Ptr<Message> output);

    DISALLOW_COPY_AND_ASSIGN(StreamFace);
};

class StreamListener : public Face {
  public:
    StreamListener(Ptr<Channel> channel);
    
    virtual bool CanSend(void) const { return false; }
    virtual bool CanReceive(void) const { return false; }
    virtual bool CanAccept() const { return true; }

  private:
    Ptr<Channel> channel_;

    //connect to channel.Accept
    //make new StreamFace and push into Accept
    void OnChannelAccept(const NetworkAddress& peer, int fd);

    DISALLOW_COPY_AND_ASSIGN(StreamListener);
};

};//namespace ndnfd
#endif//NDNFD_FACE_STREAM_H
