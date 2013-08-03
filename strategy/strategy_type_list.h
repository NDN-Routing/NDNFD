#ifdef WANT_STRATEGY_TYPE_LIST

// StrategyType_Register is automatically called when static-linked.
// It is not properly called when NDNFD is compiled as a .so and used in ns-3.
// We have to reference every available strategy here (included by strategy_type.cc),
// so that they are available when StrategyType_list is first used.

#define StrategyType_ref(cls) \
  namespace ndnfd { static const StrategyType StrategyType_##cls = cls::kType; }

#include "bcast.h"
StrategyType_ref(BcastStrategy)
#include "original.h"
StrategyType_ref(OriginalStrategy)
#include "floodfirst.h"
StrategyType_ref(FloodFirstStrategy)
#include "selflearn.h"
StrategyType_ref(SelfLearnStrategy)
#include "nacks.h"
StrategyType_ref(NacksStrategy)
#include "asl.h"
StrategyType_ref(AslStrategy)

#undef WANT_STRATEGY_TYPE_LIST
#endif//WANT_STRATEGY_TYPE_LIST
