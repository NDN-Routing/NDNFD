#ifndef NDNFD_FACE_CCND_INTERFACE_H_
#define NDNFD_FACE_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

struct face* face_from_faceid(struct ccnd_handle *h, unsigned faceid);

#ifdef __cplusplus
}
#endif
#endif//NDNFD_FACE_CCND_INTERFACE_H
