#ifndef CCND2_FACE_STREAM_H_
#define CCND2_FACE_STREAM_H_
#include "util/defs.h"
#include "face/face.h"
#include "util/pollmgr.h"
namespace ccnd2 {

class StreamFace : public Face, public IPollClient {
  public:
    StreamFace(int fd);
  
    virtual void Send(Ptr<Message> message);

    virtual void PollCallback(int fd, short revents);

  private:
    int fd_;
    ccn_charbuf* inbuf_;
    ccn_skeleton_decoder in_decoder_;
    ccn_charbuf* outbuf_;
    DISALLOW_COPY_AND_ASSIGN(StreamFace);
};

class StreamListener : public Face, public IPollClient {
  public:
    typedef boost::function2<void,int,NetworkAddress> AcceptCb;
    
    StreamListener(const NetworkAddress& local_addr, AcceptCb accept_cb);
    
    virtual bool CanSend(void) const { return false; }
    virtual bool CanReceive(void) const { return false; }
    virtual void Send(Ptr<Message> message) {}

    virtual void PollCallback(int fd, short revents);

  private:
    int fd_;
    DISALLOW_COPY_AND_ASSIGN(StreamListener);
};

};//namespace ccnd2
#endif//CCND2_FACE_STREAM_H
