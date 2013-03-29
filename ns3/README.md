# NDNFD module for ns-3

NDNFD module for ns-3 is a ns-3 module that incorperates NDNFD in simulation environment.

## How to Install

1. build NDNFD library for ns-3 simulation  
   in NDNFD directory, execute `./waf configure --sim` then `./waf`
2. [install ns-3 and ndnSIM](http://ndnsim.net/getting-started.html)
3. fix ns-3 to build in C++ 11 standard  
   in ns-3 directory, execute `grep -lr 'make_pair<' *`,
   open each file, find each `make\_pair<...> (` and replace with `make\_pair (`
4. build ns-3 with NDNFD  
   in ns-3 directory, execute `./waf configure --disable-python --enable-examples --with-ndnfd=where/is/ndnfd_source` then `waf`
5. run NDNFD simulation examples  
   in ns-3 directory, execute `NS_LOG=NDNFD:ndn.Consumer:ndn.Producer ./waf --run ndnfd-simple`

## Protocol Support

Each node has an instance of NDNFD. NDNFD instances communicate with each other over Ethernet using NDNLP wire protocol. There is no support for UDP and TCP.

NDNFD instance communicates with local apps using ndnSIM packet formats. Prefix registration is processed by a subclass of ns3::ndn::Fib.

## Programming Interface

NDNFD module for ns-3 is implemented as a subclass of ns3::ndn::L3Protocol.

* ndnSIM apps can be used with NDNFD.
* FIB, PIT, CS, and forwarding strategy are provided by ccnd core; ndnSIM equivalents cannot be used with NDNFD.

The steps to run a simulation is:

1. construct the physical topology: Nodes, Channels, NetDevices
2. defer simulation setup until after ndnfd::StackHelper::kMinStartTime().
   Due to a limitation in libccn, NDNFD cannot start before ndnfd::StackHelper::kMinStartTime().
   Call ndnfd::StackHelper::WaitUntilMinStartTime() to defer simulation setup.
3. install NDN stack with ndnfd::StackHelper
4. install NDN apps with ndnSIM ns3::ndn::AppHelper
5. set a simulation stop time with ns3::Simulator::Stop  
   Simulation will run forever without setting a stop time.
6. ns3::Simulator::Run()

