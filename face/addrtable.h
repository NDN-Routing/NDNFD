#ifndef NDNFD_FACE_ADDRTABLE_H_
#define NDNFD_FACE_ADDRTABLE_H_
#include "core/element.h"
#include "face/addrverifier.h"
namespace ndnfd {

// A FaceAddressTable records the mapping from peer address to face.
class FaceAddressTable : public Element {
 public:
  explicit FaceAddressTable(Ptr<const AddressVerifier> av) { assert(av != nullptr); this->av_ = av; }
  virtual ~FaceAddressTable(void) {}
  
  // Add remembers peer is connected on face.
  // Each peer can only be connected on one face, and each face can only connect to one peer.
  // It returns (true,FaceId_none) on success, or (false,conflicting face) on failure.
  std::tuple<bool,FaceId> Add(const NetworkAddress& peer, FaceId face);
  // Remove removes the mapping containing face.
  void Remove(FaceId face);
  
  // Find returns the FaceId that is connected to peer,
  // or FaceId_none if there isn't such a face.
  // Use the other overload if possible.
  FaceId Find(const NetworkAddress& peer) const { return this->Find(this->av_->GetHashKey(peer)); }
  
  // Find returns the FaceId that is connected to peer,
  // or FaceId_none if there isn't such a face.
  // Use this overload if possible.
  FaceId Find(const AddressHashKey& peerkey) const;

 private:
  Ptr<const AddressVerifier> av_;
  std::unordered_map<AddressHashKey,FaceId> table_;
  std::unordered_map<FaceId,AddressHashKey> rtable_;

  DISALLOW_COPY_AND_ASSIGN(FaceAddressTable);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ADDRTABLE_H
