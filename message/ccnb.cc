#include "ccnb.h"
#include "interest.h"
#include "contentobject.h"
#include "nack.h"
namespace ndnfd {

Ptr<CcnbMessage> CcnbMessage::Parse(const uint8_t* msg, size_t length) {
  ccn_skeleton_decoder decoder = {0};
  ccn_skeleton_decoder* d = &decoder;
  d->state |= CCN_DSTATE_PAUSE;
  ccn_skeleton_decode(d, msg, length);
  if (d->state < 0) return nullptr;
  if (CCN_GET_TT_FROM_DSTATE(d->state) != CCN_DTAG) return nullptr;
  ccn_dtag dtag = static_cast<ccn_dtag>(d->numval);
  switch (dtag) {
    case CCN_DTAG_Interest:
      return InterestMessage::Parse(msg, length);
    case CCN_DTAG_ContentObject:
      return ContentObjectMessage::Parse(msg, length);
    case CCN_DTAG_StatusResponse:
      return NackMessage::Parse(msg, length);
    default:
      return nullptr;
    // CCN_DTAG_CCNProtocolDataUnit is not supported, so that NDNFD cannot work with link adaptors.
    // CCN_DTAG_SequenceNumber is not supported, but the other end will detect this and stop sending seqnum.
  }
}

MessageType_def(CcnbMessage);

bool CcnbMessage::Verify(void) const {
  ccn_skeleton_decoder d;
  memset(&d, 0, sizeof(d));
  ssize_t dres = ccn_skeleton_decode(&d, this->msg(), this->length());
  return CCN_FINAL_DSTATE(d.state) && dres == static_cast<ssize_t>(this->length());
}

CcnbWireProtocol::CcnbWireProtocol(bool stream_mode) {
  this->stream_mode_ = stream_mode;
}

std::string CcnbWireProtocol::GetDescription(void) const {
  if (this->stream_mode_) return "CCNB(stream)";
  else return "CCNB(dgram)";
}

CcnbWireProtocol::State::State(void) {
  this->Clear();
}

Ptr<Buffer> CcnbWireProtocol::State::GetReceiveBuffer(void) {
  Ptr<Buffer> b = WireProtocolState::GetReceiveBuffer();
  ccn_skeleton_decoder* d = &this->d_;
  if (this->msgstart_ > 0) {
    // One or more messages have been received on this buffer.
    // We cannot overwrite this buffer because another thread may use it.
    this->CreateReceiveBuffer();
    Ptr<Buffer> b2 = this->receive_buffer();
    if (this->msgstart_ < b->length()) {
      size_t partial_length = b->length() - this->msgstart_;
      memcpy(b2->Put(partial_length), b->data() + this->msgstart_, partial_length);
      d->index -= this->msgstart_;
    } else {
      d->index = 0;
    }
    b = b2;
  }
  return b;
}

void CcnbWireProtocol::State::Clear(void) {
  memset(&this->d_, 0, sizeof(this->d_));
  this->msgstart_ = 0;
}

std::tuple<bool,std::list<Ptr<Buffer>>> CcnbWireProtocol::Encode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<const Message> message) const {
  const CcnbMessage* msg = static_cast<const CcnbMessage*>(PeekPointer(message));
  Ptr<Buffer> pkt = new Buffer(msg->length());
  memcpy(pkt->mutable_data(), msg->msg(), pkt->length());

  std::list<Ptr<Buffer>> results;
  results.push_back(pkt);
  return std::forward_as_tuple(true, results);
}

std::tuple<bool,std::list<Ptr<Message>>> CcnbWireProtocol::Decode(const NetworkAddress& peer, Ptr<WireProtocolState> state, Ptr<BufferView> packet) const {
  bool ok = true;
  State* s;
  if (this->stream_mode_) {
    assert(state != nullptr);
    s = static_cast<State*>(PeekPointer(state));
  } else {
    s = const_cast<State*>(&this->state_);
    s->Clear();
  }
  ccn_skeleton_decoder* d = &s->d_;
  std::list<Ptr<Message>> results;
  
  assert(d->index <= static_cast<ssize_t>(packet->length()));
  s->msgstart_ = 0;
  ccn_skeleton_decode(d, packet->data() + d->index, packet->length() - d->index);
  while (d->state == 0) {
    if (d->index > static_cast<ssize_t>(s->msgstart_)) {
      Ptr<CcnbMessage> msg = CcnbMessage::Parse(packet->data() + s->msgstart_, d->index - s->msgstart_);
      if (msg != nullptr) {
        //assert(msg->Verify());
        msg->set_source_buffer(packet);
        if (msg->type() == ContentObjectMessage::kType) {
          // add digest in CO because ContentStore needs it
          msg = static_cast<ContentObjectMessage*>(PeekPointer(msg))->AddExplicitDigest();
        }
        results.push_back(msg);
      }
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
