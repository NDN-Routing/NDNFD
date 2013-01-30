#ifndef CCND2_FACE_CHANNEL_H_
#define CCND2_FACE_CHANNEL_H_
#include "util/defs.h"
#include "util/buffer.h"
#include "face/face.h"
namespace ccnd2 {

class Channel : public Object {
  public:
    //whether this channel supports sending
    bool can_send(void) const { return this->can_send_; }
    //send a packet or some bytes; if blocked, remaining of pkt is copied into a queue
    virtual void Send(const NetworkAddress& peer, const Buffer* pkt) =0;
    //whether sending is blocked and must go to the queue
    bool send_blocked(void) const { return this->send_blocked_; }
    
    //whether this channel supports receiving
    bool can_receive(void) const { return this->can_receive_; }
    //receive a packet or some bytes; returns whether a packet is received
    virtual bool Receive(NetworkAddress* peer, Buffer* pkt) =0;
  
  protected:
    void set_can_send(bool value) { this->can_send_ = value; }
    void set_send_blocked(bool value) { this->send_blocked_ = value; }
    void set_can_receive(bool value) { this->can_receive_ = value; }
  
  private:
    bool can_send_;
    bool send_blocked_;
    bool can_receive_;
    DISALLOW_COPY_AND_ASSIGN(Channel);
};

class StreamChannel : public Channel {
  public:
    StreamChannel(int fd);
    
    virtual void Send(const NetworkAddress& peer, const Buffer* pkt);

    virtual bool Receive(NetworkAddress* peer, Buffer* pkt);

  private:
    int fd_;
    Buffer* outbuf_;
    DISALLOW_COPY_AND_ASSIGN(StreamChannel);
};

class DgramChannel : public Channel {
  public:
    DgramChannel(int fd, bool can_send, bool can_receive);
    
    //no sending buffer: if blocked, drop packet
    virtual void Send(const NetworkAddress& peer, const Buffer* pkt);

    virtual bool Receive(NetworkAddress* peer, Buffer* pkt);

  private:
    int fd_;
    DISALLOW_COPY_AND_ASSIGN(DgramChannel);
};

};//namespace ccnd2
#endif//CCND2_FACE_CHANNEL_H
