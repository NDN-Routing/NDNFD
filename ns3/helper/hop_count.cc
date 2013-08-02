#include "hop_count.h"
namespace ndnfd {

#define SimHopCount_magic "\xd3\x9a\x3b\xe3\xf8\x33\x4d\x43\xbb\x45\xea\x44HOP="
#define SimHopCount_size 20
#define SimHopCount_offset 16

// Hop Count can be stored in Content with at least 20 octets.
// [0..15] is the magic
// [16..19] is the hop count value, uint32_t native byte order

bool SimHopCount::Write(Ptr<ContentObjectMessage> co, uint32_t value) {
  const uint8_t* payload; size_t payload_size;
  std::tie(payload, payload_size) = co->payload();
  if (payload_size < SimHopCount_size) return false;

  if (0 != memcmp(payload, SimHopCount_magic, SimHopCount_offset)) {
    for (const uint8_t* p = payload; p < payload + payload_size; ++p) {
      if (*p != 0) return false;
    }
    memcpy(const_cast<uint8_t*>(payload), SimHopCount_magic, SimHopCount_offset);
  }
  
  const uint32_t* store = reinterpret_cast<const uint32_t*>(payload + SimHopCount_offset);
  *const_cast<uint32_t*>(store) = value;
  return true;
}

bool SimHopCount::Increment(Ptr<ContentObjectMessage> co) {
  const uint8_t* payload; size_t payload_size;
  std::tie(payload, payload_size) = co->payload();
  if (payload_size < SimHopCount_size
    || 0 != memcmp(payload, SimHopCount_magic, SimHopCount_offset))
    return false;

  const uint32_t* store = reinterpret_cast<const uint32_t*>(payload + SimHopCount_offset);
  *const_cast<uint32_t*>(store) = *store + 1;
  return true;
}

std::tuple<bool,uint32_t> SimHopCount::Read(Ptr<const ContentObjectMessage> co) {
  const uint8_t* payload; size_t payload_size;
  std::tie(payload, payload_size) = co->payload();
  if (payload_size < SimHopCount_size
    || 0 != memcmp(payload, SimHopCount_magic, SimHopCount_offset))
    return std::forward_as_tuple(false, 0);
  
  const uint32_t* store = reinterpret_cast<const uint32_t*>(payload + SimHopCount_offset);
  return std::forward_as_tuple(true, *store);
}

};//namespace ndnfd
