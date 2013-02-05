#ifndef NDNFD_FACE_WIREPROTO_H_
#define NDNFD_FACE_WIREPROTO_H_
#include "core/element.h"
#include "message/message.h"
#include "util/buffer.h"
namespace ndnfd {

// A WireProtocolState subclass is the per-peer state of a WireProtocol subclass.
class WireProtocolState : public Object {
  public:
    virtual Ptr<Buffer> GetReceiveBuffer(void);

  protected:
    Ptr<Buffer> receive_buffer(void) const { return this->receive_buffer_; }
    void set_receive_buffer(Ptr<Buffer> value) { this->receive_buffer_ = value; }

    virtual void CreateReceiveBuffer(void);

  private:
    Ptr<Buffer> receive_buffer_;
    DISALLOW_COPY_AND_ASSIGN(WireProtocolState);
};

// A WireProtocol subclass represents a wire protocol.
// A wire protocol can be stateful or stateless;
// if it is stateful, per-peer state should be stored in a WireProtocolState class.
class WireProtocol : public Element {
  public:
    // IsStateful returns true if per-peer state is needed.
    virtual bool IsStateful(void) const { return false; }
    
    // CreateState creates a new WireProtocolState object suitable for this wire protocol.
    virtual Ptr<WireProtocolState> CreateState(const NetworkAddress& peer) { assert(false); return NULL; }
    
    // Encode gets a new message, and returns zero or more packets encoded of the wire protocol.
    virtual std::vector<Ptr<Buffer>> Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Message> message);
    
    // Decode gets a new packet of the wire protocol, and returns zero or more messages.
    virtual std::vector<Ptr<Message>> Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Buffer> packet);

  private:
    DISALLOW_COPY_AND_ASSIGN(WireProtocol);
};


};//namespace ndnfd
#endif//NDNFD_FACE_WIREPROTO_H
