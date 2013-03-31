#include "ndnlp.h"
namespace ndnfd {

NdnlpWireProtocol::State::State() {
  this->pms_ = PartialMsgs_ctor();
}

NdnlpWireProtocol::State::~State() {
  PartialMsgs_dtor(this->pms_);
}

NdnlpWireProtocol::NdnlpWireProtocol(uint16_t mtu) {
  this->seqgen_ = SeqGen_ctor();
  this->slicer_ = MsgSlicer_ctor(this->seqgen_, static_cast<size_t>(mtu));
  this->ccnb_wp_ = this->New<CcnbWireProtocol>(false);
  assert(!this->ccnb_wp_->IsStateful());
}

NdnlpWireProtocol::~NdnlpWireProtocol(void) {
  MsgSlicer_dtor(this->slicer_);
  SeqGen_dtor(this->seqgen_);
}

std::tuple<bool,std::list<Ptr<Buffer>>> NdnlpWireProtocol::Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<const Message> message) const {
  bool ok; std::list<Ptr<Buffer>> ccnb_pkts;
  std::tie(ok, ccnb_pkts) = this->ccnb_wp_->Encode(peer, nullptr, message);
  
  std::list<Ptr<Buffer>> results;
  if (!ok) return std::forward_as_tuple(ok, results);
  
  for (Ptr<Buffer> ccnb_pkt : ccnb_pkts) {
    ccn_charbuf* encap = ccn_charbuf_create();
    ccnb_pkt->Swap(encap);
    CcnbMsg msg = CcnbMsg_fromEncap(encap);

    NdnlpPktA pkts = MsgSlicer_slice(this->slicer_, msg);
    for (int i = 0, len = NdnlpPktA_length(pkts); i < len; ++i) {
      NdnlpPkt pkt = NdnlpPktA_get(pkts, i);
      size_t pkt_length = NdnlpPkt_length(pkt);
      uint8_t* pkt_buf = NdnlpPkt_detachBuf(pkt);
      results.push_back(new Buffer(pkt_buf, pkt_length));
    }
    NdnlpPktA_dtor(pkts, false);
    CcnbMsg_dtor(msg);
  }
  return std::forward_as_tuple(true, results);
}

std::tuple<bool,std::list<Ptr<Message>>> NdnlpWireProtocol::Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet) const {
  assert(state != nullptr);
  State* s = static_cast<State*>(PeekPointer(state));
  std::list<Ptr<Message>> results;
  
  uint8_t* pkt_buf; size_t pkt_length;
  std::tie(pkt_buf, pkt_length) = packet->AsBuffer(false)->Detach();
  NdnlpPkt npkt = NdnlpPkt_ctor(pkt_buf, pkt_length, false);
  if (npkt == nullptr) return std::forward_as_tuple(false, results);
  DataPkt pkt = NdnlpPkt_asData(npkt);
  if (pkt == nullptr) return std::forward_as_tuple(false, results);
  
  PartialMsgRes res = PartialMsgs_arrive(s->pms_, pkt);
  if (!PartialMsgRes_isSuccess(res)) {
    NdnlpPkt_dtor(pkt);
    return std::forward_as_tuple(false, results);
  }
  if (res == PartialMsgRes_deliver) {
    CcnbMsg msg;
    while (nullptr != (msg = PartialMsgs_getDeliver(s->pms_))) {
      size_t ccnb_length;
      uint8_t* ccnb_buf = reinterpret_cast<uint8_t*>(CcnbMsg_detachBuf(msg, &ccnb_length));
      Ptr<Buffer> ccnb_pkt = new Buffer(ccnb_buf, ccnb_length);
      bool ok; std::list<Ptr<Message>> ccnb_msgs;
      std::tie(ok, ccnb_msgs) = this->ccnb_wp_->Decode(peer, nullptr, ccnb_pkt);
      if (!ok) return std::forward_as_tuple(false, results);
      results.splice(results.end(), ccnb_msgs);
    }
  }
  return std::forward_as_tuple(true, results);
}

};//namespace ndnfd
