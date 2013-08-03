#ifndef NDNFD_STRATEGY_STRATEGY_TYPE_H_
#define NDNFD_STRATEGY_STRATEGY_TYPE_H_
#include "core/element.h"
namespace ndnfd {

class Strategy;

// StrategyType indicates the type of a forwarding strategy.
typedef uint8_t StrategyType;
const StrategyType StrategyType_none = 0;
// StrategyType_inherit indicates this NPE should inherit its parent's strategy type.
const StrategyType StrategyType_inherit = 0;
// PRI_Strategy is a printf format string for Strategy.
#define PRI_StrategyType PRIu8

// StrategyCtor is a constructor of forwarding strategy.
// Global object would be copied from Element.
typedef std::function<Ptr<Strategy>(Ptr<Element>)> StrategyCtor;

StrategyType StrategyType_Register(std::string title, StrategyCtor ctor);
#define StrategyType_decl(cls) \
  static StrategyType kType; \
  static Ptr<Strategy> CreateStrategyObj(Ptr<Element> ele) { return ele->New<cls>(); } \
  virtual StrategyType strategy_type(void) const { return cls::kType; }
#define StrategyType_def(cls,title) \
  StrategyType cls::kType = StrategyType_Register( #title , &cls::CreateStrategyObj);
  //static struct cls##_reg { cls##_reg(void) { StrategyType t = cls::kType; t = t; } } cls##_regv;

// StrategyType_list returns a global, static list of forwarding strategies.
const std::map<StrategyType,std::tuple<std::string,StrategyCtor>>& StrategyType_list(void);

// StrategyType_Find returns a StrategyType that matches the title.
StrategyType StrategyType_Find(const std::string& title);

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_TYPE_H
