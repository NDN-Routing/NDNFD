#ifndef NDNFD_FACE_NDNLP_H_
#define NDNFD_FACE_NDNLP_H_
#include "face/wireproto.h"
#include "message/ccnb.h"
extern "C" {
#include "ndnld/ndnld.h"
}
namespace ndnfd {

// NdnlpWireProtocol provides NDNLP fragmentation for a datagram socket.
// This is typically used with Ethernet channel.
// It is chained with CcnbWireProtocol in stateless datagram mode.
class NdnlpWireProtocol : public WireProtocol {
 public:
  class State : public WireProtocolState {
   public:
    State();
    ~State();
    ::SeqGen* seqgen;
    ::MsgSlicer* slicer;
    ::PartialMsgs* pms;

   private:
    DISALLOW_COPY_AND_ASSIGN(State);
  };
  
  virtual bool IsStateful(void) const { return true; }
  virtual Ptr<WireProtocolState> CreateState(const NetworkAddress& peer) { return new State(); }
  
  virtual void Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Message> message, std::vector<Ptr<Buffer>>& result_packets);
  
  virtual void Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet, std::vector<Ptr<Message>>& result_messages);

 private:
  Ptr<CcnbWireProtocol> ccnb_wp_;
  DISALLOW_COPY_AND_ASSIGN(NdnlpWireProtocol);
};

};//namespace ndnfd
#endif//NDNFD_FACE_NDNLP_H
