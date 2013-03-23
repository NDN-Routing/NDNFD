# Internal Client

## ccnd internal client

The internal client is an module in ccnd. It has a similar structure as a CCNx client program. It connects to the router via face0 (router sending to face0 causes the internal client to receive a message; the messages sent by the internal client is received by the router on face0).

The internal client can process face management and prefix registration commands, publish local CCNDID, negotiate adjacency, etc. These work by calling relevant functions in ccnd.c.

## internal client as a NDNFD face

ndnfd::InternalClientFace class wraps face0.
ndnfd::InternalClientFace::Grab should be called in main loop to receive messages from the internal client.

## NDNFD as a producer in internal client

NDNFD can register a prefix on the internal client, and serve as a producer for Names under that prefix. The prefix registration is in ccnd/ndnfd\_internal\_client.h; the Interest processing is in ndnfd::InternalClientHandler class.

"ccnx:/ccnx/*CCNDID*/ndnfd" returns "NDNFD" and its version number. This serves as a signature of NDNFD.

Face Management Protocol "newface" and "destroyface" commands are served by NDNFD face manager.

