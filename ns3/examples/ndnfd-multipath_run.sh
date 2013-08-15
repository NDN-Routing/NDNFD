#!/bin/bash

project=ndnfd-multipath
mkdir -p ${project}_results

RUN_ONCE() {

result=${strategy}_q${queue_length}
echo ${result}

if [ -f ${project}.out ]; then rm ${project}.out; fi
NS_LOG=NDNFD:ndn.Consumer:ndn.Producer nohup ./waf --run ${project} --command-template="/usr/bin/time %s --ndnfd::StackHelper::RootStrategy=$strategy --ndnfd::StackHelper::SetDefaultRoutes=$set_default_routes --ns3::DropTailQueue::MaxPackets=$queue_length" &> ${project}.out
src/NDNFD/examples/${project}_plot.sh $strategy

gzip ${project}.out

resultpath=${project}_results/${result}
for f in `ls ${project}.out.gz ${project}_*.*`; do mv $f ${f/$project/$resultpath}; done

}

strategy=bcast
set_default_routes=1
queue_length=15
RUN_ONCE

strategy=original
set_default_routes=1
queue_length=15
RUN_ONCE

strategy=selflearn
set_default_routes=0
queue_length=15
RUN_ONCE

strategy=adaptive-selflearn
set_default_routes=0
queue_length=15
RUN_ONCE

