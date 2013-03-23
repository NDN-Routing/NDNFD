#ifndef NDNFD_FACE_STREAM_H_
#define NDNFD_FACE_STREAM_H_
#include "face/face.h"
#include "face/wireproto.h"
#include "face/addrverifier.h"
#include "core/pollmgr.h"
#include "face/facemgr.h"
namespace ndnfd {

// A StreamFace sends and receives messages on a stream socket.
class StreamFace : public Face, public IPollClient {
 public:
  // fd: fd of the socket, after connect() or accept()
  StreamFace(int fd, bool connecting, const NetworkAddress& peer, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~StreamFace(void);

  virtual bool CanSend(void) const { return FaceStatus_IsUsable(this->status()); }
  virtual bool CanReceive(void) const { return FaceStatus_IsUsable(this->status()); }
  
  // Send calls WireProtocol to encode the messages into octets
  // and writes them to the socket.
  virtual void Send(Ptr<Message> message);
  
  // SendBlocked returns true if send queue is not empty.
  bool SendBlocked(void) { return !this->send_queue().empty(); }

  // PollCallback is invoked with POLLIN when there are packets
  // on the socket to read.
  // PollCallback is invoked with POLLOUT when the socket is
  // available for writing.
  virtual void PollCallback(int fd, short revents);

  // SetClosing sets FaceStatus to kClosing,
  // so that face is closed after queued messages are sent.
  void SetClosing(void);
  
  virtual void Close(void) { this->Disconnect(FaceStatus::kClosed); }

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
  // If the socket is blocked for writing, or is still connecting,
  // octets are put in the send queue, and POLLOUT is registered.
  // If all octets in the send queue are sent, POLLOUT is unregistered.
  virtual void Write(void);
  
  // GetReceiveBuffer obtains a buffer for reading from socket.
  Ptr<Buffer> GetReceiveBuffer(void);
  // Read reads octets from socket, calls WireProtocol to decode as messages,
  // and writes them to Receive port.
  virtual void Read(void);
  
  // Disconnect closes fd and unregisters from pollmgr.
  void Disconnect(FaceStatus status = FaceStatus::kDisconnect);

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
  FaceKind accepted_kind(void) const { return this->accepted_kind_; }
  void set_accepted_kind(FaceKind value) { this->accepted_kind_ = value; }

  // fd: fd of the socket, after bind() and listen()
  StreamListener(int fd, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~StreamListener(void);
  
  virtual bool CanAccept(void) const { return true; }
  
  // PollCallback is invoked with POLLIN when there are
  // connection requests on the socket to accept.
  virtual void PollCallback(int fd, short revents);

  virtual void Close(void) { this->Disconnect(FaceStatus::kClosed); }

 protected:
  int fd(void) const { return this->fd_; }
  void set_fd(int value) { this->fd_ = value; }
  Ptr<AddressVerifier> av(void) const { return this->av_; }
  void set_av(Ptr<AddressVerifier> value) { this->av_ = value; }
  Ptr<WireProtocol> wp(void) const { return this->wp_; }
  void set_wp(Ptr<WireProtocol> value) { this->wp_ = value; }
  
  // AcceptConnection accepts a connection request.
  void AcceptConnection(void);
  // MakeFace makes a StreamFace from an accepted connection.
  virtual Ptr<StreamFace> MakeFace(int fd, const NetworkAddress& peer);
  
  // Disconnect closes fd and unregisters from pollmgr.
  void Disconnect(FaceStatus status = FaceStatus::kDisconnect);

 private:
  int fd_;
  Ptr<AddressVerifier> av_;
  Ptr<WireProtocol> wp_;
  FaceKind accepted_kind_;

  DISALLOW_COPY_AND_ASSIGN(StreamListener);
};

};//namespace ndnfd
#endif//NDNFD_FACE_STREAM_H
