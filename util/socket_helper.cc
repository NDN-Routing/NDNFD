#include "util/logging.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
namespace ndnfd {

bool Socket_SetNonBlock(int fd) {
  int res = fcntl(fd, F_SETFL, O_NONBLOCK);
  return res == 0;
}

int Socket_CreateForListen(int domain, int type) {
  int fd = socket(domain, type, 0);
  if (fd < 0) return fd;
  
  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if (domain == AF_INET6) {
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes));
  }
  return fd;
}

int Socket_ClearError(int fd) {
  int err = 0; socklen_t sz = sizeof(err);
  int res = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &sz);
  return res >= 0 ? err : 0;
}

};//namespace ndnfd
