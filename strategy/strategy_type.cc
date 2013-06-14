#include "strategy_type.h"
#include <limits>
namespace ndnfd {

static uint8_t StrategyType_last = 0;
static std::map<StrategyType,std::tuple<std::string,StrategyCtor>>* StrategyType_list_;
const std::map<StrategyType,std::tuple<std::string,StrategyCtor>>& StrategyType_list(void) { return *StrategyType_list_; }
StrategyType StrategyType_Register(std::string description, StrategyCtor ctor) {
  if (StrategyType_last == 0) {
    StrategyType_list_ = new std::map<StrategyType,std::tuple<std::string,StrategyCtor>>();
  }
  assert(StrategyType_last != std::numeric_limits<uint8_t>::max());
  StrategyType t = static_cast<StrategyType>(++StrategyType_last);
  (*StrategyType_list_)[t] = std::make_tuple(description,ctor);
  return t;
}

};//namespace ndnfd
