#ifndef CCND2_UTIL_POLLMGR_H_
#define CCND2_UTIL_POLLMGR_H_
#include "util/defs.h"
#include <poll.h>
namespace ccnd2 {

class IPollClient {
  public:
    virtual void PollCallback(int fd, short revents) =0;
};

class PollMgr : Object {
  public:
    void Add(Ptr<IPollClient> client, int fd, short event);
    void Remove(Ptr<IPollClient> client, int fd, short event);
    void RemoveAll(Ptr<IPollClient> client);
    
    void Poll(void);

  private:
    DISALLOW_COPY_AND_ASSIGN(PollMgr);
};

};//namespace ccnd2
#endif//CCND2_UTIL_POLLMGR_H
