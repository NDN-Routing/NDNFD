# NDNFD Strategy Management Protocol

NDNFD Strategy Management Protocol provides a method for an entity to discover the strategies supported by NDNFD, and choose a strategy for a namespace.

## List Strategies

The client expresses an Interest to `/ccnx/CCNDID/list-strategy`.

NDNFD must respond with:

	<Collection>
		<PolicyName>strategy-title</PolicyName>
	</Collection>

`<PolicyName>` is repeated once for each registered strategy. Its textual value is the title of a strategy.

An example response is:

	<Collection>
		<PolicyName>original</PolicyName>
		<PolicyName>bcast</PolicyName>
		<PolicyName>selflearn</PolicyName>
	</Collection>

## Set Strategy

The client expresses an Interest to `/ccnx/CCNDID/set-strategy/NFBLOB`. NFBLOB is the signed ContentObject in the format:

	<ForwardingEntry>
		<Action>set-strategy</Action>
		<Name>...</Name>
		<PublisherPublicKeyDigest>CCNDID</PublisherPublicKeyDigest>
		<PolicyName>strategy-title</PolicyName>
	</ForwardingEntry>

`<Name>` is the namespace; it contains zero or more `<Component>` elements. `<PolicyName>` is the title of a strategy; this strategy must be registed on NDNFD. If `<PolicyName>` is empty (contains zero char), the namespace is set to inherit the strategy of its parent.

An example request is:

	<ForwardingEntry>
		<Action>set-strategy</Action>
		<Name><Component>ndn</Component><Component>broadcast</Component></Name>
		<PublisherPublicKeyDigest>CCNDID</PublisherPublicKeyDigest>
		<PolicyName>bcast</PolicyName>
	</ForwardingEntry>

NDNFD must respond with a StatusResponse:

* StatusCode 200: success.
* StatusCode 430: ContentObject is not signed by a trusted key.
* StatusCode 405: The chosen strategy is not registered.
* StatusCode 409: Root namespace cannot inherit its parent's strategy.


