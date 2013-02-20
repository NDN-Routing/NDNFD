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

void CcnbWireProtocol::State::Clear() {
  memset(&this->d_, 0, sizeof(this->d_));
}

std::tuple<bool,std::list<Ptr<Buffer>>> CcnbWireProtocol::Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<Message> message) {
  CcnbMessage* msg = dynamic_cast<CcnbMessage*>(PeekPointer(message));
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
  size_t msgstart = 0;
  ccn_skeleton_decode(d, packet->data() + d->index, packet->length() - d->index);
  while (d->state == 0) {
    if (d->index > static_cast<ssize_t>(msgstart)) {
      results.emplace_back(new CcnbMessage(const_cast<uint8_t*>(packet->data() + msgstart), d->index - msgstart));
    }
    msgstart = d->index;
    if (msgstart == packet->length()) {
      break;
    }
    ccn_skeleton_decode(d, packet->data() + msgstart, packet->length() - msgstart);
  }
  if (d->state < 0) {
    ok = false;
  }
  if (this->stream_mode_) {
    Ptr<Buffer> rbuf = packet->AsBuffer(false);
    assert(rbuf == state->GetReceiveBuffer());
    if (msgstart < packet->length()) { 
      memmove(rbuf->mutable_data(), rbuf->data() + msgstart, rbuf->length() - msgstart);
      rbuf->Take(msgstart);
      d->index -= msgstart;
    } else {
      rbuf->Reset();
      d->index = 0;
    }
  }
  return std::forward_as_tuple(ok, results);
}

};//namespace ndnfd
