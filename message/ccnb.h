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
// It does not own the buffer.
class CcnbMessage : public Message {
 public:
  static const MessageType kType = 1099;
  CcnbMessage(uint8_t* msg, size_t length) { this->msg_ = msg; this->length_ = length; }
  virtual ~CcnbMessage(void) {}
  virtual MessageType type(void) const { return CcnbMessage::kType; }
  
  uint8_t* msg(void) const { return this->msg_; }
  size_t length(void) const { return this->length_; }
  
  // Verify checks whether CcnbMessage has correct CCNB format
  bool Verify(void) const;

 private:
  uint8_t* msg_;
  size_t length_;
  DISALLOW_COPY_AND_ASSIGN(CcnbMessage);
};

// CcnbWireProtocol provides CCNB encoding/decoding on a datagram or stream connection.
class CcnbWireProtocol : public WireProtocol {
 public:
  // A State remembers the skeleton decoder state of partial message.
  // This is only used in stream mode.
  struct State : public WireProtocolState {
    State();
    void Clear();
    ccn_skeleton_decoder d_;//skeleton decoder state
  };
  
  CcnbWireProtocol(bool stream_mode);
  
  virtual bool IsStateful(void) const { return this->stream_mode_; }
  
  virtual Ptr<WireProtocolState> CreateState(const NetworkAddress& peer) { return new State(); }
  
  virtual std::tuple<bool,std::list<Ptr<Buffer>>> Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Message> message);
  
  virtual std::tuple<bool,std::list<Ptr<Message>>> Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet);

 private:
  bool stream_mode_;
  State state_;//a State for use in dgram mode
  DISALLOW_COPY_AND_ASSIGN(CcnbWireProtocol);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CCNB_H_
