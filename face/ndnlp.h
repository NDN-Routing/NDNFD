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
    virtual ~State();
    PartialMsgs pms_;

   private:
    DISALLOW_COPY_AND_ASSIGN(State);
  };
  
  explicit NdnlpWireProtocol(uint16_t mtu);
  virtual ~NdnlpWireProtocol(void);
  
  virtual bool IsStateful(void) const { return true; }
  virtual Ptr<WireProtocolState> CreateState(const NetworkAddress& peer) const { return new State(); }
  
  virtual std::tuple<bool,std::list<Ptr<Buffer>>> Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<const Message> message) const;
  virtual std::tuple<bool,std::list<Ptr<Message>>> Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet) const;

 private:
  Ptr<CcnbWireProtocol> ccnb_wp_;
  SeqGen seqgen_;
  MsgSlicer slicer_;
  
  DISALLOW_COPY_AND_ASSIGN(NdnlpWireProtocol);
};

};//namespace ndnfd
#endif//NDNFD_FACE_NDNLP_H
