# NDNFD Strategy Layer

A forwarding strategy dictates how Interests are forwarded. For example, one forwarding strategy can broadcast Interests to all upstreams at the same time, while another may send Interests to one upstream that is believed to be the best choice.

The strategy layer allows multiple forwarding strategies to coexist in NDNFD. Each namespace (name prefix) may choose to be served by a specific strategy. Interests inside that namespace is processed by that strategy.

## Boundary of a Strategy

**Interest** is given to the strategy serving its Name (`OnInterest`).

**ContentObject** is processed by the strategy, but strategies may receive related notifications:

* After a PIT entry is satisifed, the strategy serving its Name is notified (`DidSatisfyPendingInterest`).
* After a ContentObject is processed at a name prefix entry, the strategy serving that prefix is notified (`DidReceiveContent`).
* After a unsolicited ContentObject is processed, the strategy serving its Name is notified (`DidReceiveUnsolicitedContent`).

**Interest Nack** is given to the strategy serving the Name contained in the Interest (`OnNack`).


## Implement and Register a Strategy

Each strategy is implemented as a subclass of `ndnfd::Strategy`. It must have a default constructor. The simplest strategy only needs to override `PropagateNewInterest`.

A strategy is allowed to maintain private state information on every name prefix entry, accessed via `npe->strategy_extra<T>()` and `npe->set_strategy_extra<T>(value)`. This feature is available if a strategy implements `NewNpeExtra`, `InheritNpeExtra`, and `FinalizeNpeExtra`. In case a new strategy is chosen for a namespace (and therefore name prefix entries), old strategy is invoked to finalize its private states, and new strategy is invoked to create private states. A strategy should not maintain states elsewhere (eg. on the strategy object).

A strategy must be registered so that it can be used. `StrategyType_decl` and `StrategyType_def` macros are used for this purpose. A registered strategy has a strategy type (for internal use), and a short human-readable description. `StrategyType_list` function returns all registered strategies.



