#ifndef NDNFD_FACE_FACEID_H_
#define NDNFD_FACE_FACEID_H_
#include <sys/types.h>
namespace ndnfd {

// A FaceId is a number that identifies a Face within a router.
// It is assigned by FaceMgr.
typedef int16_t FaceId;

// PRI_FaceId is a printf format string for FaceId.
#define PRI_FaceId PRId16

// FaceId_none indicates no FaceId is assigned.
static const FaceId FaceId_none = -1;


// A NetworkAddress represents the address in any lower protocol.
struct NetworkAddress {
  sockaddr_storage who;
  socklen_t wholen;
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEID_H
