#ifndef CCND2_FACE_CCNBDECODER_H_
#define CCND2_FACE_CCNBDECODER_H_
#include "util/defs.h"
#include <deque>
#include "message/interest.h"
#include "message/contentobject.h"
namespace ccnd2 {

class CcnbDecoder : Object {
  public:
    CcnbDecoder(void);
    
    //get the first decoded message; NULL if none is ready
    boost::shared_ptr<Message> Get(void);
    
  private:
    std::deque<Message*> parsed_;
    DISALLOW_COPY_AND_ASSIGN(CcnbDecoder);
};

class CcnbStreamDecoder {
  public:
    CcnbStreamDecoder(void);
    
    //whether there's an protocol error: if true, stream should be closed
    bool HasError(void) const;
    
    //feed input into decoder
    //buf is owned by CcnbStreamDecoder
    void Input(uint8_t* buf, size_t length);

  private:
    ccn_charbuf* buf_;
    DISALLOW_COPY_AND_ASSIGN(CcnbStreamDecoder);
};

class CcnbDgramDecoder : public CcnbDecoder {
  public:
    CcnbDgramDecoder(void);
    
    //feed one frame of input into decoder
    //buf is owned by CcnbFrameDecoder
    //silently drop malformed frames
    void Input(uint8_t* buf, size_t length);

  private:
    ccn_charbuf* buf_;
    DISALLOW_COPY_AND_ASSIGN(CcnbDgramDecoder);
};

};//namespace ccnd2
#endif//CCND2_FACE_CCNBDECODER_H_
