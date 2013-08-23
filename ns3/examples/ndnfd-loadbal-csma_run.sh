#!/bin/bash

project=ndnfd-loadbal-csma
mkdir -p ${project}_results

RUN_ONCE() {

result=${strategy}_${n_consumers}_${frequency}_${process_time}ms_${n_producers}_${queue_capacity}
utilization=$((${n_consumers}*${frequency}*${process_time}*100/${n_producers}/1000))
echo ${result} ${utilization}%

randomize_param=''
if [ ''$randomize -eq 1 ]; then randomize_param='--ns3::ndn::ConsumerCbr::Randomize=uniform'; fi

if [ -f ${project}.out ]; then rm ${project}.out; fi
NS_LOG=NDNFD:ndn.Consumer:ndn.ConsumerWindow:ndn.ProducerThrottled nohup ./waf --run ${project} --command-template="/usr/bin/time %s --ndnfd::StackHelper::RootStrategy=$strategy --ndnfd::StackHelper::SetDefaultRoutes=$set_default_routes --n_producers=$n_producers --ns3::ProcessingDelay::QueueCapacity=$queue_capacity --ns3::ProcessingDelay::ProcessTime=${process_time}ms --n_consumers=$n_consumers --maxseq=$maxseq --frequency=$frequency --sim_time=$sim_time $randomize_param" &> ${project}.out
src/NDNFD/examples/${project}_plot.sh $n_producers $process_time $n_consumers $maxseq $sim_time

gzip ${project}.out

resultpath=${project}_results/${result}
for f in `ls ${project}.out.gz ${project}_*.*`; do mv $f ${f/$project/$resultpath}; done

}


strategy=adaptive-selflearn
set_default_routes=0
n_producers=2
queue_capacity=5
process_time=20
n_consumers=4
frequency=15
maxseq=512
sim_time=900
randomize=1


RUN_ONCE

