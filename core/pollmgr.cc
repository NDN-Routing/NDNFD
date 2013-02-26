#include "core/pollmgr.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
namespace ndnfd {

PollMgr::PollMgr(void) {
  this->pfds_ = nullptr;
  this->nfds_ = this->pfds_limit_ = 0;
}

PollMgr::~PollMgr(void) {
  if (this->pfds_ != nullptr) free(this->pfds_);
}

void PollMgr::Add(IPollClient* client, int fd, short events) {
  events = events & PollMgr::kEvents;
  
  Reg& reg = this->regs_[fd];
  reg.clients_[client] = reg.clients_[client] | events;

  this->UpdateRegEvents(&reg);
  this->UpdatePfds();
}

void PollMgr::Remove(IPollClient* client, int fd, short events) {
  events = events & PollMgr::kEvents;
  
  Reg& reg = this->regs_[fd];
  reg.clients_[client] = reg.clients_[client] & ~events;

  this->UpdateRegEvents(&reg);
  this->UpdatePfds();
}

void PollMgr::RemoveAll(IPollClient* client) {
  for (auto it = this->regs_.begin(); it != this->regs_.end(); ++it) {
    if (0 != it->second.clients_.erase(client)) {
      this->UpdateRegEvents(&it->second);
    }
  }
  this->UpdatePfds();
}

void PollMgr::UpdatePfds(void) {
  nfds_t nfds = this->regs_.size();
  if (this->pfds_limit_ < nfds) {
    this->pfds_limit_ = 1;
    while (this->pfds_limit_ < nfds) this->pfds_limit_ = this->pfds_limit_ << 1;
    if (this->pfds_ != nullptr) free(this->pfds_);
    this->pfds_ = static_cast<pollfd*>(malloc(this->pfds_limit_ * sizeof(pollfd)));
  }
  nfds = 0;
  for (auto it = this->regs_.cbegin(); it != this->regs_.cend();) {
    auto me = it; ++it;
    if (me->second.clients_.size() == 0) {
      this->regs_.erase(me);
      continue;
    }
    this->pfds_[nfds].fd = me->first;
    this->pfds_[nfds].events = me->second.events_;
    ++nfds;
  }
  this->nfds_ = nfds;
}

void PollMgr::UpdateRegEvents(Reg* reg) {
  short events = 0;
  for (auto it = reg->clients_.begin(); it != reg->clients_.end();) {
    auto me = it; ++it;
    if (me->second == 0) {
      reg->clients_.erase(me);
    } else {
      events |= me->second;
    }
  }
  reg->events_ = events;
}

bool PollMgr::Poll(std::chrono::milliseconds timeout) {
  int timeout_ms = static_cast<int>(timeout.count());
  int r = poll(this->pfds_, this->nfds_, timeout_ms);
  if (r == -1) return false;
  if (r == 0) return true;
  
  this->PollSuccess();
  //this->Log(kLLDebug, kLCPollMgr, "PollMgr::poll -> %d", r);

  std::vector<std::tuple<IPollClient*,int,short>> calls;
  
  auto it = this->regs_.cbegin();
  for (nfds_t i = 0; i < this->nfds_; ++i) {
    const pollfd& pfd = this->pfds_[i];
    int fd = pfd.fd; short revents = pfd.revents;
    //this->Log(kLLDebug, kLCPollMgr, "PollMgr::pollfd[] = %d,%x,%x", fd, pfd.events, revents);
    if (revents == 0) continue;
    while (it != this->regs_.cend() && it->first < fd) ++it;
    if (it == this->regs_.cend()) break;
    if (it->first != fd) continue;

    for (auto itc = it->second.clients_.cbegin(); itc != it->second.clients_.cend(); ++itc) {
      if (0 != (revents & (itc->second | PollMgr::kErrors))) {
        calls.push_back(std::make_tuple(itc->first, fd, revents));
        // don't call PollCallback here - it may modify pfd and regs
      }
    }
  }
  
  for (auto call : calls) {
    IPollClient* client = std::get<0>(call);
    int fd = std::get<1>(call);
    short revents = std::get<2>(call);
    client->PollCallback(fd, revents);
  }
  return true;
}

void PollMgr::PollSuccess(void) {
  ccnd_handle* h = this->global()->ccndh();
  if (h->ticktock.gettime == nullptr) return;//in unittest
  // refresh ccnd time
  ccn_timeval dummy;
  h->ticktock.gettime(&h->ticktock, &dummy);
}

};//namespace ndnfd
