#ifndef NDNFD_STRATEGY_STRATEGY_TYPE_H_
#define NDNFD_STRATEGY_STRATEGY_TYPE_H_
#include "core/element.h"
namespace ndnfd {

class Strategy;

typedef uint8_t StrategyType;
static const StrategyType StrategyType_inherit = 0;

typedef std::function<Ptr<Strategy>(Ptr<Element>)> StrategyCtor;
StrategyType StrategyType_Register(std::string description, StrategyCtor ctor);
#define StrategyType_decl(cls) \
  static const StrategyType kType; \
  static Ptr<Strategy> CreateStrategyObj(Ptr<Element> ele) { return ele->New<cls>(); }
#define StrategyType_def(cls,description) \
  const StrategyType cls::kType = StrategyType_Register( #description , &cls::CreateStrategyObj);

const std::map<StrategyType,std::tuple<std::string,StrategyCtor>>& StrategyType_list(void);

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_TYPE_H
