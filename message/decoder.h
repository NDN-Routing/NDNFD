#ifndef NDNFD_MESSAGE_DECODER_H_
#define NDNFD_MESSAGE_DECODER_H_
#include "core/element.h"
#include "message/message.h"
namespace ndnfd {

class Decoder : public Element {
  public:
    enum DecoderMode {
      kDMStream = 1,
      kDMDgram = 2
    };
    Decoder(Mode mode) { this->mode_ = mode; }
    
    //feed a packet into the decoder
    virtual void Input(uint8_t* buf, size_t length);
    
    //called for each decoded messages
    PushPort<Ptr<Message>> Output;
    
    //called if there's an error
    PushPort<bool> Error;
  protected:
    enum DecodeStatus {
      kDSNone = 0,
      kDSSuccess = 1,//successfully decoded one message
      kDSPartial = 2,//the message is incomplete in this buffer
      kDSError = 3//there's an error: Dgram-mode - drop this packet; Stream-mode - stream should be closed due to this error
    };
    struct DecodeResult {
      DecodeStatus status;
      Ptr<Message> message;//decoded message, if status==kDSSuccess
      size_t consumed;//consumed bytes, if status==kDSSuccess or kDSPartial
    };

    DecoderMode mode(void) const { return this->mode_; }

    //decode one message
    //start is usually 0
    //for Stream-mode:
    //  if last Decode returns kDSPartial, the partial message is saved into inbuf_
    //  decoder should maintain states about the parsed portion
    virtual DecodeResult Decode(uint8_t* buf, size_t length, size_t start) =0;

  private:
    DecoderMode stream_mode_;
    Buffer inbuf_;

    DISALLOW_COPY_AND_ASSIGN(Decoder);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_DECODER_H_
