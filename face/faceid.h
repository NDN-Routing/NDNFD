#ifndef NDNFD_FACE_FACEID_H_
#define NDNFD_FACE_FACEID_H_
#include "util/defs.h"
#include <sys/socket.h>
namespace ndnfd {

// A FaceId is a number that identifies a Face within a router.
// It is assigned by FaceMgr.
typedef int32_t FaceId;

// PRI_FaceId is a printf format string for FaceId.
#define PRI_FaceId PRId32

// FaceId_none indicates no FaceId is assigned.
static const FaceId FaceId_none = -1;


// A NetworkAddress represents the address in any lower protocol.
struct NetworkAddress {
  NetworkAddress(void) { memset(this, 0, sizeof(this)); this->wholen = sizeof(this->who); }
  sa_family_t family(void) const { return reinterpret_cast<const sockaddr*>(&this->who)->sa_family; }
  sockaddr_storage who;
  socklen_t wholen;
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEID_H
