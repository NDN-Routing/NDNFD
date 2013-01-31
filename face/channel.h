#ifndef NDNFD_FACE_CHANNEL_H_
#define NDNFD_FACE_CHANNEL_H_
#include "core/element.h"
#include "face/faceid.h"
#include "util/buffer.h"
namespace ndnfd {

//a channel that can send and/or receive through a socket
//owns the socket: dtor should close the socket
class Channel : public Element {
  public:
    class IReceiveBufferProvider {
      public:
        Ptr<Buffer> ObtainReceiveBuffer(void) =0;
        void ReleaseReceiveBuffer(Ptr<Buffer> buf, bool receive_was_success) =0;
    }

    //whether this channel allows sending
    bool CanSend(void) const { return this->can_send_; }
    //whether sending is blocked and will go to the queue
    bool IsSendBlocked(void) const { return this->send_blocked_; }
    //send a packet or some bytes; if blocked, remaining of pkt is copied into a queue
    virtual void Send(const NetworkAddress& peer, Ptr<Buffer> pkt) {}
    
    //whether this channel can receive
    bool CanReceive(void) const { return this->can_receive_; }
    //receive a packet or some bytes
    PushPort<const NetworkAddress&,Ptr<Buffer>> Receive;
    //register a receive buffer provider
    //returns true on success, false if this channel does not support rbp
    //  a channel that supports rbp must:
    //  1. call rbp->ObtainReceiveBuffer() before receive
    //  2. read from socket into the end of buffer
    //  3. if a packet is received, increase buffer length by the number of octets read, push to Receive port, and call rbp->ReleaseReceiveBuffer(buf,true)
    //  4. if no packet is received, call rbp->ReleaseReceiveBuffer(buf,false)
    //  the channel must not hold the buffer for any longer
    bool RegisterReceiveBufferProvider(IReceiveBufferProvider rbp);
    
    //whether this channel can accept new connections
    bool CanAccept(void) const { return this->can_accept_; }
    //accept a new connection
    PushPort<const NetworkAddress&,int> Accept;
    
    //socket error
    PushPort<bool> Error;
    
  protected:
    //a dummy receive buffer provider useful for channel supports rbp but none is registered
    class DummyReceiveBufferProvider : public IReceiveBufferProvider {
      public:
        Ptr<Buffer> ObtainReceiveBuffer(void) { return new Buffer(); }
        void ReleaseReceiveBuffer(Ptr<Buffer> buf, bool receive_was_success) {}
    };
  
    void set_can_send(bool value) { this->can_send_ = value; }
    void set_send_blocked(bool value) { this->send_blocked_ = value; }
    void set_can_receive(bool value) { this->can_receive_ = value; }

    //whether this channel supports receive buffer provider
    virtual bool SupportReceiveBufferProvider() { return false; }
  
  private:
    bool can_send_;
    bool send_blocked_;
    bool can_receive_;
    DISALLOW_COPY_AND_ASSIGN(Channel);
};

class StreamChannel : public Channel, public IPollClient {
  public:
    StreamChannel(int fd);
    
    virtual void Send(const NetworkAddress& peer, const Buffer* pkt);

    virtual void PollCallback(int fd, short revents);
  
  private:
    Buffer* outbuf_;
    DISALLOW_COPY_AND_ASSIGN(StreamChannel);
};

class StreamListener : public Channel, public IPollClient {
  public:
    StreamListener(int fd);
    
    virtual void PollCallback(int fd, short revents);
  
  private:
    DISALLOW_COPY_AND_ASSIGN(StreamListener);
};

class DgramChannel : public Channel, public IPollClient {
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
