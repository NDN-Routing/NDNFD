#ifndef NDNFD_CORE_POLLMGR_H_
#define NDNFD_CORE_POLLMGR_H_
#include "core/defs.h"
#include <poll.h>
namespace ndnfd {

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

};//namespace ndnfd
#endif//NDNFD_CORE_POLLMGR_H
