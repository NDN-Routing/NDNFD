#ifndef NDNFD_CORE_POLLMGR_H_
#define NDNFD_CORE_POLLMGR_H_
#include "core/element.h"
#include <poll.h>
#include "util/thread.h"
#include "util/lockfreequeue.h"
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
  
  // kZeroTimeout specifies Poll() should return immediately.
  static constexpr std::chrono::milliseconds kZeroTimeout = std::chrono::milliseconds(0);
  // kNoTimeout specifies Poll() should wait forever.
  static constexpr std::chrono::milliseconds kNoTimeout = std::chrono::milliseconds(-1);
 
  PollMgr(void);
  virtual ~PollMgr(void);

  // Poll must be invoked on local_thread.
  // Add/Remove/RemoveAll can be invoked on any thread.
  std::thread::id local_thread(void) const { return this->local_thread_; }
  void set_local_thread(std::thread::id value) { this->local_thread_ = value; }
  
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
  
  // local_thread is the thread on which Poll is called.
  // If Add/Remove/RemoveAll are called on another thread, they are accumulated
  // in local_calls and deferred until the beginning of next Poll.
  std::thread::id local_thread_;
  LockFreeQueue<std::function<void(void)>> local_calls_;
  
  // UpdatePfds updates pfds_ according to Reg.events_.
  void UpdatePfds(void);
  // UpdateRegEvents updates Reg.events_ according to Reg.clients_.
  void UpdateRegEvents(Reg* reg);
  
  // PollSuccess is called after poll() syscall returns,
  // but before dispatching events.
  void PollSuccess(void);
  
  DISALLOW_COPY_AND_ASSIGN(PollMgr);
};

};//namespace ndnfd
#endif//NDNFD_CORE_POLLMGR_H
