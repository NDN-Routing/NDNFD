#include "stream.h"
#include <unistd.h>
#include "util/socket_helper.h"
namespace ndnfd {

StreamFace::StreamFace(int fd, bool connecting, const NetworkAddress& peer, Ptr<WireProtocol> wp) {
  assert(fd > 0);
  assert(wp != nullptr);
  this->set_fd(fd);
  this->set_status(connecting ? FaceStatus::kConnecting : FaceStatus::kUndecided);
  this->set_wp(wp);
  this->set_peer(peer);
}

void StreamFace::Init(void) {
  if (this->wp()->IsStateful()) {
    this->set_wps(this->wp()->CreateState(this->peer()));
  } else {
    this->wps_ = nullptr;
  }
  this->inbuf_ = nullptr;
  this->global()->facemgr()->AddFace(this);
  this->global()->pollmgr()->Add(this, this->fd(), POLLIN);
  //this->Log(kLLInfo, kLCFace, "StreamFace(%"PRIxPTR")::StreamFace fd=%d status=%s", this, this->fd(), FaceStatus_ToString(this->status()).c_str());
}

StreamFace::~StreamFace(void) {
  this->global()->pollmgr()->RemoveAll(this);
  ::close(this->fd());
}

void StreamFace::Send(Ptr<Message> message) {
  if (!FaceStatus_IsUsable(this->status())) {
    this->Log(kLLError, kLCFace, "StreamFace(%"PRIxPTR")::Send but status is %s", this, FaceStatus_ToString(this->status()).c_str());
    return;
  }
  
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
  if ((revents & PollMgr::kErrors) != 0) {
    close(this->fd());
    this->set_status(FaceStatus::kDisconnect);
    return;
  }
  if ((revents & POLLOUT) != 0) {
    this->Write();
  }
  if ((revents & POLLIN) != 0) {
    this->Read();
  }
}

void StreamFace::SetClosing(void) {
  if (FaceStatus_IsUsable(this->status())) {
    this->set_status(FaceStatus::kClosing);
  }
  if (this->status() == FaceStatus::kClosing && !this->SendBlocked()) {
    close(this->fd());
    this->set_status(FaceStatus::kClosed);
  }
}

void StreamFace::Enqueue(std::list<Ptr<Buffer>>* pkts) {
  this->send_queue().splice(this->send_queue().end(), *pkts);
}

void StreamFace::Write(void) {
  while (!this->send_queue().empty()) {
    Ptr<Buffer>& pkt = this->send_queue().front();
    ssize_t res = write(this->fd(), pkt->data(), pkt->length());
    if (res < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        close(this->fd());
        this->set_status(FaceStatus::kDisconnect);
        return;
      }
      break;
    } else {
      if (this->status() == FaceStatus::kConnecting) {
        this->set_status(FaceStatus::kUndecided);
      }
      if (static_cast<size_t>(res) < pkt->length()) {//socket is blocked
        pkt->Pull(static_cast<size_t>(res));
        break;
      } else {//pkt sent in full
        this->send_queue().pop_front();
      }
    }
  }
  if (this->SendBlocked()) {
    this->global()->pollmgr()->Add(this, this->fd(), POLLOUT);
  } else {
    this->global()->pollmgr()->Remove(this, this->fd(), POLLOUT);
    if (this->status() == FaceStatus::kClosing) {
      close(this->fd());
      this->set_status(FaceStatus::kClosed);
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
  int sockerr = Socket_ClearError(this->fd());
  if (sockerr == ETIMEDOUT && this->status() == FaceStatus::kConnecting) {
    close(this->fd());
    this->set_status(FaceStatus::kConnectError);
  }

  Ptr<Buffer> pkt = this->GetReceiveBuffer();
  const size_t bufsize = 1<<20;
  ssize_t res = read(this->fd(), pkt->Reserve(bufsize), bufsize);
  if (res < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      close(this->fd());
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

  if (!msgs.empty()) this->set_status(FaceStatus::kEstablished);
  for (Ptr<Message> msg : msgs) {
    this->Receive(msg);
  }
}

StreamListener::StreamListener(int fd, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) {
  assert(fd > 0);
  assert(av != nullptr);
  assert(wp != nullptr);
  this->set_fd(fd);
  this->set_av(av);
  this->set_wp(wp);
}

void StreamListener::Init(void) {
  this->global()->facemgr()->AddFace(this);
  this->global()->pollmgr()->Add(this, this->fd(), POLLIN);
}

StreamListener::~StreamListener(void) {
  this->global()->pollmgr()->RemoveAll(this);
  ::close(this->fd());
}

void StreamListener::PollCallback(int fd, short revents) {
  assert(fd == this->fd());
  if ((revents & PollMgr::kErrors) != 0) {
    close(this->fd());
    this->set_status(FaceStatus::kDisconnect);
    return;
  }
  if ((revents & POLLIN) != 0) {
    this->AcceptConnection();
  }
}

void StreamListener::AcceptConnection(void) {
  NetworkAddress peer;
  int fd = accept(this->fd(), (struct sockaddr*)&peer.who, &peer.wholen);
  if (fd < 0) return;
  if (!this->av()->Check(peer)) {
    this->Log(kLLError, kLCFace, "StreamListener(%"PRIxPTR")::AcceptConnection peer address not valid", this);
    close(fd);
    return;
  }
  Ptr<StreamFace> face = this->MakeFace(fd, peer);
  this->Log(kLLInfo, kLCFace, "StreamListener(%"PRIxPTR")::AcceptConnection fd=%d face=%"PRIxPTR" peer=%s", this, fd, PeekPointer(face), this->av()->ToString(peer).c_str());
  this->Accept(face);
}

Ptr<StreamFace> StreamListener::MakeFace(int fd, const NetworkAddress& peer) {
  if (!Socket_SetNonBlock(fd)) {
    this->Log(kLLWarn, kLCFace, "StreamListener(%"PRIxPTR")::MakeFace fd=%d O_NONBLOCK %s", this, fd, Logging::ErrorString().c_str());
  }
  
  NetworkAddress normalized = this->av()->Normalize(peer);
  Ptr<StreamFace> face = this->New<StreamFace>(fd, false, normalized, this->wp());
  return face;
}

void StreamListener::Close(void) {
  close(this->fd());
  this->set_status(FaceStatus::kClosed);
}

};//namespace ndnfd
