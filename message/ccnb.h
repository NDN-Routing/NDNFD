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
  CcnbMessage(const uint8_t* msg, size_t length) { this->msg_ = msg; this->length_ = length; }
  virtual ~CcnbMessage(void) {}
  MessageType_decl;
  
  // Parse parses a CCNB message into a specific subtype,
  // or returns null.
  static Ptr<CcnbMessage> Parse(const uint8_t* msg, size_t length);
  
  // CCNB message
  const uint8_t* msg(void) const { return this->msg_; }
  // length of msg
  size_t length(void) const { return this->length_; }
  
  // Verify checks whether CcnbMessage has correct CCNB format
  bool Verify(void) const;
  
  // add reference to a BufferView so that it won't be deleted before this CcnbMessage
  void set_source_buffer(Ptr<const BufferView> value) { this->source_buffer_ = value; }
  
 private:
  const uint8_t* msg_;
  size_t length_;
  
  Ptr<const BufferView> source_buffer_;
  
  DISALLOW_COPY_AND_ASSIGN(CcnbMessage);
};

// CcnbWireProtocol provides CCNB encoding/decoding on a datagram or stream connection.
class CcnbWireProtocol : public WireProtocol {
 public:
  // A State remembers the skeleton decoder state of partial message.
  // This is only used in stream mode.
  struct State : public WireProtocolState {
    State(void);
    virtual Ptr<Buffer> GetReceiveBuffer(void);
    void Clear(void);
    size_t msgstart_;//start position of first undelivered message in receive buffer
    ccn_skeleton_decoder d_;//skeleton decoder state
  };
  
  explicit CcnbWireProtocol(bool stream_mode);
  virtual std::string GetDescription(void) const;
  
  virtual bool IsStateful(void) const { return this->stream_mode_; }
  virtual Ptr<WireProtocolState> CreateState(const NetworkAddress& peer) const { return new State(); }
  
  virtual std::tuple<bool,std::list<Ptr<Buffer>>> Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<const Message> message) const;
  virtual std::tuple<bool,std::list<Ptr<Message>>> Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet) const;

 private:
  bool stream_mode_;
  State state_;//a State for use in dgram mode
  DISALLOW_COPY_AND_ASSIGN(CcnbWireProtocol);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CCNB_H_
