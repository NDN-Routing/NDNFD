#ifndef NDNFD_FACE_FACEID_H_
#define NDNFD_FACE_FACEID_H_
#include <sys/types.h>
namespace ndnfd {

typedef int16_t FaceId;
#define PRI_FaceId PRId16//printf format string
static const FaceId FaceId_none = -1;

struct NetworkAddress {
  sockaddr_storage who;
  socklen_t wholen;
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEID_H
