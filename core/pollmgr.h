#ifndef NDNFD_CORE_POLLMGR_H_
#define NDNFD_CORE_POLLMGR_H_
#include "core/element.h"
#include <chrono>
#include <poll.h>
namespace ndnfd {

// An IPollClient implementor can receive events from PollMgr.
class IPollClient {
 public:
  virtual void PollCallback(int fd, short revents) =0;
};

// PollMgr runs poll() syscall on behalf of clients.
//   Clients must:
//   1. make sure to call RemoveAll before dtor
//   2. be prepared for error events POLLERR POLLHUP POLLNVAL
//   3. register POLLOUT only when there's nothing to write
class PollMgr : public Element {
 public:
  // events that can be registered
  static const short kEvents = POLLIN | POLLOUT;
  // error events that are always delivered
  static const short kErrors = POLLERR | POLLHUP | POLLNVAL;
  
  PollMgr(void);
  virtual ~PollMgr(void);
  
  // Add makes client to be invoked if event occurs on fd.
  void Add(IPollClient* client, int fd, short events);
  
  // Remove marks client not longer waiting for event on fd.
  void Remove(IPollClient* client, int fd, short events);
  
  // RemoveAll unregisters all events for client.
  void RemoveAll(IPollClient* client);
  
  // Poll runs poll() syscall and invokes clients.
  // Returns true on success, false on error.
  bool Poll(std::chrono::milliseconds timeout);

 private:
  // A Reg lists the clients interested in an fd.
  struct Reg {
    std::unordered_map<IPollClient*, short> clients_;
    short events_;
  };
  // registrations by fd
  std::map<int,Reg> regs_;
  
  // struct pollfd[]
  pollfd* pfds_;
  // number of pollfds in use
  nfds_t nfds_;
  // number of pollfds allocated
  nfds_t pfds_limit_;
  
  // UpdatePfds updates pfds_ according to Reg.events_.
  void UpdatePfds(void);
  // UpdateRegEvents updates Reg.events_ according to Reg.clients_.
  void UpdateRegEvents(Reg* reg);
  
  DISALLOW_COPY_AND_ASSIGN(PollMgr);
};

};//namespace ndnfd
#endif//NDNFD_CORE_POLLMGR_H
