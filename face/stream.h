#ifndef NDNFD_FACE_STREAM_H_
#define NDNFD_FACE_STREAM_H_
#include "face/face.h"
#include "face/wireproto.h"
#include "face/addrverifier.h"
#include "core/pollmgr.h"
namespace ndnfd {

// A StreamFace sends and receives messages on a stream socket.
//
// If the socket is blocked for writing, or is still connecting,
// octets are put in the send queue, and POLLOUT is registered.
// If all octets in the send queue are sent, POLLOUT is unregistered.
class StreamFace : public Face, public IPollClient {
 public:
  // fd: fd of the socket, after connect() or accept()
  StreamFace(int fd, Ptr<WireProtocol> wp);
  virtual ~StreamFace(void) {}

  virtual bool CanSend(void) const { return this->status() != FaceStatus::kError; }
  virtual bool CanReceive(void) const { return this->status() != FaceStatus::kError; }

  // Send calls WireProtocol to encode the messages into octets
  // and writes them to the socket.
  virtual void Send(Ptr<Message> message);

  // PollCallback is invoked with POLLIN when there are packets
  // on the socket to read.
  // PollCallback is invoked with POLLOUT when the socket is
  // available for writing.
  virtual void PollCallback(int fd, short revents);

 protected:
  // DeferredWrite writes contents in send queue into the socket,
  // until socket is blocked again, or the send queue is empty.
  virtual void DeferredWrite(void);

 private:
  Ptr<WireProtocol> wp_;
  Ptr<WireProtocolState> wps_;
  std::queue<Ptr<Buffer>> send_queue_;

  DISALLOW_COPY_AND_ASSIGN(StreamFace);
};

// A StreamListener listens for new connections on a stream socket.
class StreamListener : public Face, public IPollClient {
 public:
  // fd: fd of the socket, after bind() and listen()
  StreamListener(int fd, Ptr<AddressVerifier> av);
  virtual ~StreamListener(void) {}
  
  virtual bool CanAccept() const { return true; }
  
  // PollCallback is invoked with POLLIN when there are
  // connection requests on the socket to accept.
  virtual void PollCallback(int fd, short revents);

 private:
  int fd_;
  Ptr<AddressVerifier> av_;

  DISALLOW_COPY_AND_ASSIGN(StreamListener);
};

};//namespace ndnfd
#endif//NDNFD_FACE_STREAM_H
