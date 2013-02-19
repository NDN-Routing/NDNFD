#include "stream.h"
#include <unistd.h>
namespace ndnfd {

StreamFace::StreamFace(int fd, const NetworkAddress& peer, Ptr<WireProtocol> wp) {
  assert(fd > 0);
  assert(wp != nullptr);
  this->set_fd(fd);
  this->set_wp(wp);
  if (wp->IsStateful()) {
    this->set_wps(wp->CreateState(peer));
  } else {
    this->wps_ = nullptr;
  }
  this->inbuf_ = nullptr;
  this->set_peer(peer);
}

void StreamFace::Send(Ptr<Message> message) {
  if (!FaceStatus_IsUsable(this->status())) return;
  
  bool ok; std::list<Ptr<Buffer>> pkts;
  std::tie(ok, pkts) = this->wp()->Encode(this->peer(), this->wps(), message);
  if (!ok) {
    this->set_status(FaceStatus::kProtocolError);
    return;
  }
  
  this->Enqueue(&pkts);
  this->Write();
}

void StreamFace::PollCallback(int fd, short revents) {
  assert(fd == this->fd());
  if ((revents & POLLOUT) != 0) {
    this->Write();
  }
  if ((revents & POLLIN) != 0) {
    this->Read();
  }
}

void StreamFace::Enqueue(std::list<Ptr<Buffer>>* pkts) {
  this->send_queue().splice(this->send_queue().end(), *pkts);
}

void StreamFace::Write(void) {
  if (this->status() == FaceStatus::kConnecting) return;
  
  while (!this->send_queue().empty()) {
    Ptr<Buffer>& pkt = this->send_queue().front();
    ssize_t res = ::write(this->fd(), pkt->data(), pkt->length());
    if (res < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        this->set_status(FaceStatus::kDisconnect);
      }
      return;
    } else if (static_cast<size_t>(res) < pkt->length()) {//socket is blocked
      pkt->Pull(static_cast<size_t>(res));
      return;
    } else {//pkt sent in full
      this->send_queue().pop_front();
    }
  }
}

Ptr<Buffer> StreamFace::GetReceiveBuffer(void) {
  if (this->wps() != nullptr) {
    return this->wps()->GetReceiveBuffer();
  } else {
    if (this->inbuf() == nullptr) this->inbuf() = new Buffer(0);
    this->inbuf()->Reset();
    return this->inbuf();
  }
}

void StreamFace::Read(void) {
  Ptr<Buffer> pkt = this->GetReceiveBuffer();
  const size_t bufsize = 1<<20;
  ssize_t res = ::read(this->fd(), pkt->Reserve(bufsize), bufsize);
  if (res < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      this->set_status(FaceStatus::kDisconnect);
    }
    return;
  }
  pkt->Put(static_cast<size_t>(res));

  bool ok; std::list<Ptr<Message>> msgs;
  std::tie(ok, msgs) = this->wp()->Decode(this->peer(), this->wps(), pkt);
  if (!ok) {
    this->set_status(FaceStatus::kProtocolError);
    return;
  }

  if (this->status() == FaceStatus::kUndecided) this->set_status(FaceStatus::kEstablished);
  for (auto it = msgs.begin(); it != msgs.end(); ++it) {
    this->Receive(*it);
  }
}

};//namespace ndnfd
