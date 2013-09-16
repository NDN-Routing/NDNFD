# NDNFD module for ns-3

NDNFD module for ns-3 is a ns-3 module that incorperates NDNFD in simulation environment.

## How to Install

NDNFD module for ns-3 works on linux.  
It compiles on OS X, but does not work.

1. build NDNFD library for ns-3 simulation (`$NDNFD_ROOT` represents the location of NDNFD directory)

		cd $NDNFD_ROOT
		./waf configure --sim
		./waf

2. [install ns-3 and ndnSIM](http://ndnsim.net/getting-started.html) (`$NS3_ROOT` represents the location of ns-3 directory)

		git clone git://github.com/cawka/ns-3-dev-ndnSIM.git $NS3_ROOT
		cd $NS3_ROOT
		git checkout ns-3.17-ndnSIM-0.5
		git clone git://github.com/NDN-Routing/ndnSIM.git src/ndnSIM
		cd src/ndnSIM
		git checkout 0aa47bf4bf9f786dff24663cb31f988fab4882b4

3. install NDNFD module into ns-3

		cd $NS3_ROOT/src
		ln -s $NDNFD_ROOT/ns3 NDNFD

4. build ns-3 with NDNFD  

		# linux
		cd $NS3_ROOT
		./waf configure --disable-python --enable-examples --with-ndnfd=$NDNFD_ROOT
		./waf
		
		# OS X
		cd $NS3_ROOT
		git apply src/NDNFD/clang_ns-3.17-ndnSIM-0.5.patch
		CC=clang CXX=clang++ ./waf configure --disable-python --enable-examples --with-ndnfd=$NDNFD_ROOT
		./waf

5. run NDNFD simulation examples  

		cd $NS3_ROOT
		NS_LOG=NDNFD:ndn.Consumer:ndn.Producer ./waf --run ndnfd-simple

## Protocol Support

Each node has an instance of NDNFD. NDNFD instances communicate with each other over Ethernet using CCNB wire protocol. There is no support for UDP and TCP.

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
   Simulation will run forever if stop time is not set.
6. ns3::Simulator::Run()

