#include "addrtable.h"
namespace ndnfd {

std::tuple<bool,FaceId> FaceAddressTable::Add(const NetworkAddress& peer, FaceId face) {
  AddressHashKey peerkey = this->av_->GetHashKey(peer);
  auto conflict = this->table_.find(peerkey);
  if (conflict != this->table_.end()) {
    return std::forward_as_tuple(false, conflict->second);
  }
  this->table_[peerkey] = face;
  this->rtable_[face] = peerkey;
  return std::forward_as_tuple(true, FaceId_none);
}

void FaceAddressTable::Remove(FaceId face) {
  auto it_peerkey = this->rtable_.find(face);
  if (it_peerkey == this->rtable_.end()) {
    this->Log(kLLWarn, kLCFaceMgr, "FaceAddressTable::Remove(%" PRI_FaceId ") face not in table", face);
    return;
  }
  
  const AddressHashKey& peerkey = it_peerkey->second;
  this->table_.erase(peerkey);
  this->rtable_.erase(it_peerkey);
}

FaceId FaceAddressTable::Find(const AddressHashKey& peerkey) const {
  auto it = this->table_.find(peerkey);
  if (it == this->table_.end()) {
    return FaceId_none;
  }
  return it->second;
}

};//namespace ndnfd
