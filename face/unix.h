#ifndef CCND2_FACE_UNIX_H_
#define CCND2_FACE_UNIX_H_
#include "face/stream.h"
namespace ccnd2 {

class UnixFaceMaster : public FaceMaster {
  public:
    UnixFaceMaster(const std::string& local_socket);

    virtual Ptr<Face> listener(void) { return this->listener_; }

    virtual Ptr<Face> multicast(void) { return NULL; }
    
    virtual Ptr<Face> unicast(NetworkAddress peer) { return NULL; }
    
    void ListenerAccept(int fd, NetworkAddress peer);
  private:
    Ptr<StreamListener> listener_;
    
    DISALLOW_COPY_AND_ASSIGN(UnixFaceMaster);
};

};//namespace ccnd2
#endif//CCND2_FACE_UNIX_H
