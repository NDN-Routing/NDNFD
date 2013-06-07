# Interest NACK

**Interest NACK** is a new message type introduced in the adaptive forwarding scheme described in paper *A case for stateful forwarding plane*. NDNFD recognizes Interest NACK messages as `ndnfd::NackMessage`.

## CCNB encoding

An Interest NACK is encoded in CCNB as:

	<StatusResponse>
		<StatusCode>161</StatusCode>
		<StatusText></StatusText>
		<Interest>
			....
		</Interest>
	</StatusResponse>

`<StatusCode>` may contain a three digit code in range \[161,169\], and the following codes are defined:

* `161` represents NACK code *Duplicate*
* `162` represents NACK code *Congestion*
* `163` represents NACK code *No Data*

`<StatusText>` element is optional and its value is ignored.

Interest NACK is encoded as well-formed CCNB. Other CCN software that does not recognize Interest NACK will ignore such messages.

