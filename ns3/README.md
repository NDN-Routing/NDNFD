# NDNFD module for ns-3

NDNFD module for ns-3 is a ns-3 module that incorperates NDNFD in simulation environment.

## How to Install

1. build NDNFD library for ns-3 simulation (`$NDNFD_ROOT` represents the location of NDNFD directory)

		cd $NDNFD_ROOT
		./waf configure --sim
		./waf

2. [install ns-3 and ndnSIM](http://ndnsim.net/getting-started.html) (`$NS3_ROOT` represents the location of ns-3 directory)

		git clone git://github.com/cawka/ns-3-dev-ndnSIM.git $NS3_ROOT
		git clone git://github.com/NDN-Routing/ndnSIM.git $NS3_ROOT/src/ndnSIM
		cd $NS3_ROOT/src/ndnSIM
		git checkout v0.5-rc1

3. fix ns-3 to build in C++ 11 standard

		cd $NS3_ROOT
		grep -lr 'make_pair<' *
   open each file, find each `make_pair<...> (` and replace with `make_pair (`

4. install NDNFD module into ns-3

		cd $NS3_ROOT/src
		ln -s $NDNFD_ROOT/ns3 NDNFD

5. build ns-3 with NDNFD  

		cd $NS3_ROOT
		./waf configure --disable-python --enable-examples --with-ndnfd=$NDNFD_ROOT
		./waf

6. run NDNFD simulation examples  

		cd $NS3_ROOT
		NS_LOG=NDNFD:ndn.Consumer:ndn.Producer ./waf --run ndnfd-simple

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

