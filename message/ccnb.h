#ifndef NDNFD_MESSAGE_CCNB_H_
#define NDNFD_MESSAGE_CCNB_H_
#include "util/defs.h"
extern "C" {
#include <ccn/ccn.h>
}
#include "message/message.h"
#include "face/wireproto.h"
namespace ndnfd {

// A CcnbMessage represents an unparsed CCNB message.
// It can be passed to process_input_message() for processing.
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

// A CcnbWireProtocol reads and writes CCNB messages on a datagram or stream connection.
class CcnbWireProtocol : public WireProtocol {
  public:
    // A State is used when CcnbWireProtocol is used on a stream connection,
    // so that partial message is decoded only once at this level.
    struct State : public WireProtocolState {
      size_t last_length;//previous length of buffer; the difference is new arrival
      ::ccn_skeleton_decoder decoder;//skeleton decoder state
    };
    
    public CcnbWireProtocol(bool stream);
    
    virtual bool IsStateful(void) const { return this->stream_; }
    
    virtual Ptr<WireProtocolState> CreateState(const NetworkAddress& peer) { return new State(); }
    
    virtual void Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Message> message, std::vector<Ptr<Buffer>>& result_packets);
    
    virtual void Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Buffer> packet, std::vector<Ptr<Message>>& result_message

  private:
    bool stream_;
    DISALLOW_COPY_AND_ASSIGN(CcnbWireProtocol);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CCNB_H_
