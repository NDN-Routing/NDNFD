#include "ccnb.h"
namespace ndnfd {

bool CcnbMessage::Verify(void) const {
  ccn_skeleton_decoder d;
  memset(&d, 0, sizeof(d));
  ssize_t dres = ccn_skeleton_decode(&d, this->msg(), this->length());
  return CCN_FINAL_DSTATE(d.state) && dres == static_cast<ssize_t>(this->length());
}

CcnbWireProtocol::CcnbWireProtocol(bool stream_mode) {
  this->stream_mode_ = stream_mode;
}

CcnbWireProtocol::State::State() {
  this->Clear();
}

Ptr<Buffer> CcnbWireProtocol::State::GetReceiveBuffer(void) {
  Ptr<Buffer> b = WireProtocolState::GetReceiveBuffer();
  ccn_skeleton_decoder* d = &this->d_;
  if (this->msgstart_ < b->length()) { 
    b->Pull(this->msgstart_);
    b->Rebase();
    d->index -= this->msgstart_;
  } else {
    b->Reset();
    d->index = 0;
  }
  return b;
}

void CcnbWireProtocol::State::Clear() {
  memset(&this->d_, 0, sizeof(this->d_));
  this->msgstart_ = 0;
}

std::tuple<bool,std::list<Ptr<Buffer>>> CcnbWireProtocol::Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Message> message) {
  CcnbMessage* msg = static_cast<CcnbMessage*>(PeekPointer(message));
  Ptr<Buffer> pkt = new Buffer(msg->length());
  memcpy(pkt->mutable_data(), msg->msg(), pkt->length());

  std::list<Ptr<Buffer>> results;
  results.push_back(pkt);
  return std::forward_as_tuple(true, results);
}

std::tuple<bool,std::list<Ptr<Message>>> CcnbWireProtocol::Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet) {
  bool ok = true;
  State* s;
  if (this->stream_mode_) {
    assert(state != nullptr);
    s = static_cast<State*>(PeekPointer(state));
  } else {
    s = &this->state_;
    s->Clear();
  }
  ccn_skeleton_decoder* d = &s->d_;
  std::list<Ptr<Message>> results;
  
  assert(d->index <= static_cast<ssize_t>(packet->length()));
  s->msgstart_ = 0;
  ccn_skeleton_decode(d, packet->data() + d->index, packet->length() - d->index);
  while (d->state == 0) {
    if (d->index > static_cast<ssize_t>(s->msgstart_)) {
      Ptr<CcnbMessage> msg = new CcnbMessage(const_cast<uint8_t*>(packet->data() + s->msgstart_), d->index - s->msgstart_);
      assert(msg->Verify());
      msg->source_buffer_ = packet;
      results.push_back(msg);
    }
    s->msgstart_ = d->index;
    if (s->msgstart_ == packet->length()) {
      break;
    }
    ccn_skeleton_decode(d, packet->data() + s->msgstart_, packet->length() - s->msgstart_);
  }
  if (d->state < 0) {
    ok = false;
  }
  return std::forward_as_tuple(ok, results);
}

};//namespace ndnfd
