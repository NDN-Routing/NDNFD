#ifndef NDNFD_NS3_HELPER_HOP_COUNT_H_
#define NDNFD_NS3_HELPER_HOP_COUNT_H_
#include "message/contentobject.h"
namespace ndnfd {

// HopCount class stores hop count with a magic number in ContentObject.
class SimHopCount {
 public:
  // Write writes hop count magic number into Content
  // if length permits and Content is completely empty.
  static bool Write(Ptr<ContentObjectMessage> co, uint32_t value);
  // Increment adds one to hop count, if exists.
  static bool Increment(Ptr<ContentObjectMessage> co);
  // Read reads hop count, if exists.
  // Otherwise, false is returned.
  static std::tuple<bool,uint32_t> Read(Ptr<const ContentObjectMessage> co);
  
 private:
  SimHopCount(void) {}
};

};//namespace ndnfd
#endif//NDNFD_NS3_HELPER_HOP_COUNT_H_
