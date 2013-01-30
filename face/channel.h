#ifndef NDNFD_FACE_CHANNEL_H_
#define NDNFD_FACE_CHANNEL_H_
#include "core/element.h"
#include "face/faceid.h"
#include "util/buffer.h"
namespace ndnfd {

//a channel that can send and/or receive through a socket
//owns the socket: dtor should close the socket
class Channel : public Element, public IPollClient {
  public:
    //whether this channel supports sending
    bool CanSend(void) const { return this->can_send_; }
    //whether sending is blocked and will go to the queue
    bool IsSendBlocked(void) const { return this->send_blocked_; }
    //send a packet or some bytes; if blocked, remaining of pkt is copied into a queue
    virtual void Send(const NetworkAddress& peer, Ptr<Buffer> pkt) {}
    
    //whether this channel supports receiving
    bool CanReceive(void) const { return this->can_receive_; }
    //receive a packet or some bytes
    PushPort<std::pair<NetworkAddress,Ptr<Buffer>>> Receive;
    
  protected:
    void set_can_send(bool value) { this->can_send_ = value; }
    void set_send_blocked(bool value) { this->send_blocked_ = value; }
    void set_can_receive(bool value) { this->can_receive_ = value; }
  
  private:
    int fd_;
    bool can_send_;
    bool send_blocked_;
    bool can_receive_;
    DISALLOW_COPY_AND_ASSIGN(Channel);
};

class StreamChannel : public Channel {
  public:
    StreamChannel(int fd);
    
    virtual void Send(const NetworkAddress& peer, const Buffer* pkt);

    virtual void PollCallback(int fd, short revents);
  
  private:
    Buffer* outbuf_;
    DISALLOW_COPY_AND_ASSIGN(StreamChannel);
};

class StreamListenChannel : public Channel {
  public:
    StreamListenChannel(int fd);
    
    PushPort<std::pair<int,NetworkAddress>> Accept;

    virtual void PollCallback(int fd, short revents);
  
  private:
    DISALLOW_COPY_AND_ASSIGN(StreamListenChannel);
};

class DgramChannel : public Channel {
  public:
    DgramChannel(int fd, bool can_send, bool can_receive);
    
    //no sending buffer: if blocked, drop packet
    virtual void Send(const NetworkAddress& peer, const Buffer* pkt);

    virtual void PollCallback(int fd, short revents);

  private:
    DISALLOW_COPY_AND_ASSIGN(DgramChannel);
};

};//namespace ndnfd
#endif//NDNFD_FACE_CHANNEL_H
