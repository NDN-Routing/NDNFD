#ifndef NDNFD_UTIL_SOCKET_HELPER_H_
#define NDNFD_UTIL_SOCKET_HELPER_H_
#include "util/defs.h"
namespace ndnfd {

// Socket_SetNonBlock puts a socket to non-blocking mode,
// and returns true on success.
bool Socket_SetNonBlock(int fd);

// Socket_CreateForListen creates a socket that is suitable for listening.
int Socket_CreateForListen(int domain, int type);

// Socket_ClearError clears the pending error in socket,
// and returns the error.
int Socket_ClearError(int fd);

};//namespace ndnfd
#endif//NDNFD_UTIL_SOCKET_HELPER_H_
