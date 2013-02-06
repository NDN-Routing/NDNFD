#ifndef NDNFD_CORE_POLLMGR_H_
#define NDNFD_CORE_POLLMGR_H_
#include "core/element.h"
#include <poll.h>
namespace ndnfd {

// A IPollClient implementor can receive events from PollMgr.
class IPollClient {
  public:
    virtual void PollCallback(int fd, short revents) =0;
};

// A PollMgr is the central place of poll() syscall.
class PollMgr : public Element {
  public:
    // Add makes client to be invoked if event occurs on fd.
    void Add(Ptr<IPollClient> client, int fd, short event);
    
    // Remove marks client not longer waiting for event on fd.
    void Remove(Ptr<IPollClient> client, int fd, short event);
    
    // RemoveAll unregisters all events for client.
    void RemoveAll(Ptr<IPollClient> client);
    
    // Poll runs poll() syscall and invokes clients.
    void Poll(void);

  private:
    DISALLOW_COPY_AND_ASSIGN(PollMgr);
};

};//namespace ndnfd
#endif//NDNFD_CORE_POLLMGR_H
