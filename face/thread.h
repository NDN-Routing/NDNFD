#ifndef NDNFD_FACE_THREAD_H_
#define NDNFD_FACE_THREAD_H_
#include "core/pollmgr.h"
#include "face/faceid.h"
#include "message/message.h"
#include "util/lockfreequeue.h"
namespace ndnfd {

class Face;

// A FaceThread is a container of several faces.
class FaceThread : public Element, public IPollClient {
 public:
  FaceThread(void);
  virtual void Init(void);
  virtual ~FaceThread(void);
  bool IsFaceThread(void) const;
  
  // Stop informs and waits for face thread to stop.
  virtual void Stop(void);
  
  Ptr<PollMgr> pollmgr(void) const { return this->pollmgr_; }
  
  void NewFace(Ptr<Face> face);
  
  // Send delivers a message from router core to a face.
  // This is invoked in router thread.
  virtual void Send(FaceId face, Ptr<Message> message);
  
  // Receive gets a message from a face to router core.
  // This is triggered in router thread.
  PushPort<Ptr<Message>> Receive;
  
  // FaceReceive puts a message from a face to router core.
  // This is invoked in face thread.
  virtual void FaceReceive(Ptr<Message> message);

  // PollCallback is invoked with POLLIN when either notify_fds is notified.
  virtual void PollCallback(int fd, short revents);

 protected:
  std::thread::id face_thread_id_;
  Ptr<PollMgr> pollmgr_;

 private:
  std::thread face_thread_;
  std::atomic_bool running_;

  int notify_fds_[2];// [0] in core thread pollmgr, [1] in face thread pollmgr
  LockFreeQueue<std::tuple<FaceId,Ptr<Message>>> out_queue_;// messages to send
  LockFreeQueue<Ptr<Message>> in_queue_;// received messages
  
  void RunFaceThread();
  void PullOutQueue();
  void PullInQueue();
    
  DISALLOW_COPY_AND_ASSIGN(FaceThread);
};

// An IntegratedFaceThread behaves like FaceThread but executes within core thread.
class IntegratedFaceThread : public FaceThread {
 public:
  IntegratedFaceThread(void) {}
  virtual void Init(void);
  virtual ~IntegratedFaceThread(void) {}

  virtual void Stop(void) {}
  virtual void Send(FaceId face, Ptr<Message> message);
  virtual void FaceReceive(Ptr<Message> message);
  virtual void PollCallback(int fd, short revents) { assert(false); }

 private:
  DISALLOW_COPY_AND_ASSIGN(IntegratedFaceThread);
};

};//namespace ndnfd
#endif//NDNFD_FACE_THREAD_H
