#!/bin/bash

n_producers=2
queue_capacity=5
process_time=10
frequency='window'
maxseq=4096
sim_time=900

RUN_ONCE() {

result_dir=${frequency}_${process_time}ms_${n_producers}_${queue_capacity}
echo $result_dir

file_size=`python2 -c "print $maxseq / 1024.0"`
NS_LOG=NDNFD:ndn.Consumer:ndn.ConsumerWindow:ndn.ProducerThrottled nohup ./waf --run ndnfd-loadbal-csma --command-template="/usr/bin/time %s --n_producers=$n_producers --ns3::ProcessingDelay::QueueCapacity=$queue_capacity --ns3::ProcessingDelay::ProcessTime=${process_time}ms --ns3::ndn::ConsumerCbr::MaxSeq=$maxseq --ns3::ndn::ConsumerWindow::Size=$file_size --frequency=$frequency --sim_time=$sim_time" &> ndnfd-loadbal-csma.out
src/NDNFD/examples/ndnfd-loadbal-csma_plot.sh $n_producers $process_time $maxseq $sim_time

mkdir -p $result_dir
mv ndnfd-loadbal-csma* $result_dir/

}

RUN_ONCE

