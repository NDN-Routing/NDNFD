#ifndef CCND2_FACE_FACEID_H_
#define CCND2_FACE_FACEID_H_
#include "util/defs.h"
#include <sys/types.h>
namespace ccnd2 {

typedef int16_t FaceId;
#define PRI_FaceId PRId16//printf format string
static const FaceId FaceId_none = -1;

struct NetworkAddress {
  sockaddr_storage who;
  socklen_t wholen;
};

};//namespace ccnd2
#endif//CCND2_FACE_FACEID_H
