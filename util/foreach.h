#ifndef NDNFD_UTIL_FOREACH_H_
#define NDNFD_UTIL_FOREACH_H_
namespace ndnfd {

enum class ForeachAction {
  kNone   = 0,
  kBreak  = 1,// stop iterating
  kDelete = 2,// delete current record (not supported everywhere)
  kBreakDelete = kBreak | kDelete
};

#define FOREACH_CONTINUE { return ForeachAction::kNone; }
#define FOREACH_BREAK    { return ForeachAction::kBreak; }
#define FOREACH_OK       FOREACH_CONTINUE

inline bool ForeachAction_break(ForeachAction act) { return (static_cast<int>(act) & static_cast<int>(ForeachAction::kBreak)) != 0; }
inline bool ForeachAction_delete(ForeachAction act) { return (static_cast<int>(act) & static_cast<int>(ForeachAction::kDelete)) != 0; }

};//namespace ndnfd
#endif//NDNFD_UTIL_FOREACH_H_
