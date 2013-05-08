#!/bin/bash

n_producers=1
queue_capacity=5
process_time=10
n_consumers=5
frequency=15
maxseq=2048
sim_time=900
randomize='--ns3::ndn::ConsumerCbr::Randomize=uniform'

RUN_ONCE() {

result_dir=${n_consumers}_${frequency}_${process_time}ms_${n_producers}_${queue_capacity}
echo $result_dir

NS_LOG=NDNFD:ndn.Consumer:ndn.ConsumerWindow:ndn.ProducerThrottled nohup ./waf --run ndnfd-loadbal-csma --command-template="/usr/bin/time %s --n_producers=$n_producers --ns3::ProcessingDelay::QueueCapacity=$queue_capacity --ns3::ProcessingDelay::ProcessTime=${process_time}ms --n_consumers=$n_consumers --maxseq=$maxseq --frequency=$frequency --sim_time=$sim_time $randomize" &> ndnfd-loadbal-csma.out
src/NDNFD/examples/ndnfd-loadbal-csma_plot.sh $n_producers $process_time $n_consumers $maxseq $sim_time

gzip ndnfd-loadbal-csma.out
mkdir -p $result_dir
mv ndnfd-loadbal-csma* $result_dir/

}

RUN_ONCE

