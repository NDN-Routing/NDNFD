#ifndef CCND2_FACE_MASTER_H_
#define CCND2_FACE_MASTER_H_
#include "util/defs.h"
#include <unordered_set>
#include "face/face.h"
#include "util/pollmgr.h"
namespace ccnd2 {

//master of faces sharing the same local binding
class FaceMaster : public Object {
  public:
    typedef boost::function1<void,Ptr<Face>> AcceptCb;
    
    //get or create the listener face
    virtual Ptr<Face> listener(void) =0;
    
    //get or create the multicast face
    virtual Ptr<Face> multicast(void) =0;
    
    //get or create a unicast face to specified peer
    virtual Ptr<Face> unicast(PeerAddress peer) =0;

    //register a callback for accepted unicast faces
    void set_accept_cb(AcceptCb value) { this->accept_cb_ = value; }
    
    //send a message
    virtual void Send(Ptr<Message> message);
    
  protected:
    void set_id(FaceId value) { this->id_ = value; }
    void call_accept_cb(boost::shared_ptr<Face> face) { if (this->accept_cb() != NULL) this->accept_cb()(message); }

  private:
    AcceptCb accept_cb_;

    AcceptCb accept_cb(void) const { return this->accept_cb_; }
    
    DISALLOW_COPY_AND_ASSIGN(FaceMaster);
};

};//namespace ccnd2
#endif//CCND2_FACE_MASTER_H
