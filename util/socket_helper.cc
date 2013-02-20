#include "util/logging.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
namespace ndnfd {

bool Socket_SetNonBlock(int fd) {
  int res = fcntl(fd, F_SETFL, O_NONBLOCK);
  return res == 0;
}

int Socket_ClearError(int fd) {
  int err = 0; socklen_t sz = sizeof(err);
  int res = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &sz);
  return res >= 0 ? err : 0;
}

};//namespace ndnfd
