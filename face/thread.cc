#include "thread.h"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "face/facemgr.h"
namespace ndnfd {

FaceThread::FaceThread(void) {
  this->pollmgr_ = nullptr;
  this->notify_fds_[0] = this->notify_fds_[1] = -1;
}

void FaceThread::Init(void) {
  assert(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, this->notify_fds_));
  this->pollmgr_ = this->New<PollMgr>();
  fcntl(this->notify_fds_[0], F_SETFL, O_NONBLOCK);
  this->global()->pollmgr()->Add(this, this->notify_fds_[0], POLLIN);
  fcntl(this->notify_fds_[1], F_SETFL, O_NONBLOCK);
  this->pollmgr()->Add(this, this->notify_fds_[1], POLLIN);

  this->running_ = true;
  this->face_thread_ = std::thread(std::mem_fn(&FaceThread::RunFaceThread), this);
}

FaceThread::~FaceThread(void) {
  this->Stop();
  if (this->notify_fds_[0] != -1) {
    close(this->notify_fds_[0]);
    close(this->notify_fds_[1]);
  }
}

bool FaceThread::IsFaceThread(void) const {
  return std::this_thread::get_id() == this->face_thread_id_;
}

void FaceThread::Stop(void) {
  assert(this->global()->IsCoreThread());
  bool running_expected = true;
  if (this->running_.compare_exchange_strong(running_expected, false)) {
    write(this->notify_fds_[0], ".", 1);
    this->face_thread_.join();
  }
  this->global()->pollmgr()->RemoveAll(this);
}

void FaceThread::NewFace(Ptr<Face> face) {
  assert(this->global()->IsCoreThread());
  this->Log(kLLDebug, kLCFace, "FaceThread(%" PRIxPTR ")::NewFace(%" PRIxPTR ")", this, PeekPointer(face));
  face->Receive = std::bind(&FaceThread::FaceReceive, this, std::placeholders::_1);
  if (this->notify_fds_[0] != -1) {
    write(this->notify_fds_[0], ".", 1);
  }
}

void FaceThread::Send(FaceId face, Ptr<Message> message) {
  assert(this->global()->IsCoreThread());
  this->Log(kLLDebug, kLCFace, "FaceThread(%" PRIxPTR ")::Send(%" PRI_FaceId ")", this, face);
  this->out_queue_.push(std::make_tuple(face, message));
  write(this->notify_fds_[0], ".", 1);
}

void FaceThread::FaceReceive(Ptr<Message> message) {
  assert(this->IsFaceThread());
  this->Log(kLLDebug, kLCFace, "FaceThread(%" PRIxPTR ")::FaceReceive(%" PRI_FaceId ")", this, message->incoming_face());
  this->in_queue_.push(message);
  write(this->notify_fds_[1], ".", 1);
}

void FaceThread::RunFaceThread() {
  this->face_thread_id_ = std::this_thread::get_id();
  this->pollmgr()->set_local_thread(this->face_thread_id_);
  while (this->running_) {
    this->Log(kLLDebug, kLCFace, "FaceThread(%" PRIxPTR ")::RunFaceThread() poll", this);
    this->pollmgr()->Poll(PollMgr::kNoTimeout);
  }
  this->pollmgr()->RemoveAll(this);
}

void FaceThread::PollCallback(int fd, short revents) {
  uint8_t buf[32];
  if (fd == this->notify_fds_[0]) {
    assert(this->global()->IsCoreThread());
    read(fd, buf, sizeof(buf));
    this->PullInQueue();
  } else if (fd == this->notify_fds_[1]) {
    assert(this->IsFaceThread());
    read(fd, buf, sizeof(buf));
    this->PullOutQueue();
  }
}

void FaceThread::PullOutQueue() {
  assert(this->IsFaceThread());
  int count = 0;
  bool ok; std::tuple<FaceId,Ptr<Message>> outgoing;
  while (std::tie(ok, outgoing) = this->out_queue_.pop(), ok) {
    ++count;
    // TODO make FaceMgr::GetFace lock-free
    Ptr<Face> face = this->global()->facemgr()->GetFace(std::get<0>(outgoing));
    if (face == nullptr) continue;
    assert(face->face_thread() == this);
    if (!face->CanSend()) continue;
    Ptr<Message> message = std::get<1>(outgoing);
    //this->Log(kLLWarn, kLCFace, "FaceThread(%" PRIxPTR ")::PullOutQueue() Face(%" PRI_FaceId ")::Send(type=%" PRIu32 ")", this, face->id(), message->type());
    face->Send(message);
  }
  this->Log(kLLDebug, kLCFace, "FaceThread(%" PRIxPTR ")::PullOutQueue() %d messages", this, count);
}

void FaceThread::PullInQueue() {
  assert(this->global()->IsCoreThread());
  int count = 0;
  bool ok; Ptr<Message> message;
  while (std::tie(ok, message) = this->in_queue_.pop(), ok) {
    ++count;
    this->Receive(message);
  }
  this->Log(kLLDebug, kLCFace, "FaceThread(%" PRIxPTR ")::PullInQueue() %d messages", this, count);
}

void IntegratedFaceThread::Init(void) {
  assert(this->global()->IsCoreThread());
  this->face_thread_id_ = std::this_thread::get_id();
  this->pollmgr_ = this->global()->pollmgr();
}

void IntegratedFaceThread::Send(FaceId face, Ptr<Message> message) {
  assert(this->global()->IsCoreThread());
  this->Log(kLLDebug, kLCFace, "IntegratedFaceThread(%" PRIxPTR ")::Send(%" PRI_FaceId ")", this, face);
  Ptr<Face> f = this->global()->facemgr()->GetFace(face);
  if (f == nullptr) return;
  assert(f->face_thread() == this);
  if (!f->CanSend()) return;
  f->Send(message);
}

void IntegratedFaceThread::FaceReceive(Ptr<Message> message) {
  assert(this->global()->IsCoreThread());
  this->Log(kLLDebug, kLCFace, "IntegratedFaceThread(%" PRIxPTR ")::FaceReceive(%" PRI_FaceId ")", this, message->incoming_face());
  this->Receive(message);
}

};//namespace ndnfd
