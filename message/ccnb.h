#ifndef NDNFD_MESSAGE_CCNB_H_
#define NDNFD_MESSAGE_CCNB_H_
#include "util/defs.h"
extern "C" {
#include <ccn/ccn.h>
}
#include "message/message.h"
#include "message/decoder.h"
#include "message/encoder.h"
#include "face/channel.h"
namespace ndnfd {

//unparsed ccnb message
//can pass to process_input_message()
class CcnbMessage : public Message {
  public:
    static const MessageType kType = 1099;
    virtual MessageType type(void) const { return CcnbMessage::kType; }
    
    //CcnbMessage does not own this buffer
    uint8_t* msg;
    size_t size;

  private:
    DISALLOW_COPY_AND_ASSIGN(CcnbMessage);
};

class CcnbDecoder : public Decoder, public Channel::IReceiveBufferProvider {
  public:
    Ptr<Buffer> ObtainReceiveBuffer(void) =0;
    void ReleaseReceiveBuffer(Ptr<Buffer> buf, bool receive_was_success) =0;

  protected:
    virtual DecodeResult Decode(uint8_t* buf, size_t length, size_t start);
    
  private:
    DISALLOW_COPY_AND_ASSIGN(CcnbDecoder);
};

class CcnbEncoder : public Encoder {
  protected:
    virtual void Encode(Ptr<Message> message, std::function<void(Ptr<Buffer>)> emit);
    
  private:
    DISALLOW_COPY_AND_ASSIGN(CcnbEncoder);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CCNB_H_
