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
  StreamFace(int fd, const NetworkAddress& peer, Ptr<WireProtocol> wp);
  virtual ~StreamFace(void) {}

  virtual bool CanSend(void) const { return FaceStatus_IsUsable(this->status()); }
  virtual bool CanReceive(void) const { return FaceStatus_IsUsable(this->status()); }

  // Send calls WireProtocol to encode the messages into octets
  // and writes them to the socket.
  virtual void Send(Ptr<Message> message);

  // PollCallback is invoked with POLLIN when there are packets
  // on the socket to read.
  // PollCallback is invoked with POLLOUT when the socket is
  // available for writing.
  virtual void PollCallback(int fd, short revents);

 protected:
  int fd(void) const { return this->fd_; }
  void set_fd(int value) { this->fd_ = value; }
  Ptr<WireProtocol> wp(void) const { return this->wp_; }
  void set_wp(Ptr<WireProtocol> value) { this->wp_ = value; }
  Ptr<WireProtocolState> wps(void) const { return this->wps_; }
  void set_wps(Ptr<WireProtocolState> value) { this->wps_ = value; }
  const NetworkAddress& peer(void) const { return this->peer_; }
  void set_peer(const NetworkAddress& value) { this->peer_ = value; }
  Ptr<Buffer> inbuf(void) const { return this->inbuf_; }
  void set_inbuf(Ptr<Buffer> value) { this->inbuf_ = value; }
  std::list<Ptr<Buffer>>& send_queue(void) { return this->send_queue_; }
  
  // Enqueue appends pkts into send queue.
  void Enqueue(std::list<Ptr<Buffer>>* pkts);
  // Write writes octets from send queue into the socket,
  // until socket is blocked, or queue is empty.
  virtual void Write(void);
  
  // GetReceiveBuffer obtains a buffer for reading from socket.
  Ptr<Buffer> GetReceiveBuffer(void);
  // Read reads octets from socket, calls WireProtocol to decode as messages,
  // and writes them to Receive port.
  virtual void Read(void);

 private:
  int fd_;
  Ptr<WireProtocol> wp_;
  Ptr<WireProtocolState> wps_;
  NetworkAddress peer_;
  Ptr<Buffer> inbuf_;
  std::list<Ptr<Buffer>> send_queue_;

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
