#ifndef NDNFD_MESSAGE_ENCODER_H_
#define NDNFD_MESSAGE_ENCODER_H_
#include "core/element.h"
#include "message/message.h"
namespace ndnfd {

class Encoder : public Element {
  public:
    //max size of a packet
    typedef int32_t Mtu;
    //a special MTU for stream channels
    static const Mtu kStream = -1;
    
    Encoder(Mtu mtu) { this->mtu_ = mtu; }
    
    //feed a message into the encoder
    virtual void Input(const NetworkAddress& receiver, Ptr<Message> message);
    
    //called for each encoded packet
    PushPort<const NetworkAddress&,Ptr<Buffer>> Output;

  protected:
    Mtu mtu(void) const { return this->mtu_; }
    
    //encode the message, and emit each packet
    virtual void Encode(Ptr<Message> message, std::function<void(Ptr<Buffer>)> emit);
    
  private:
    Mtu mtu_;
    
    //passed to Encode
    void EncodeEmit(const NetworkAddress& receiver, Ptr<Buffer> pkt) { this->Output(receiver, pkt); }

    DISALLOW_COPY_AND_ASSIGN(Decoder);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_ENCODER_H_
