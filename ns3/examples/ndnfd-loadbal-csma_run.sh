#!/bin/bash

n_producers=4
queue_capacity=10
process_time=10
frequency=300
sim_time=10
stop_req_time=$(($sim_time-1))

producer_util=$(($process_time * $frequency * 100 / ($n_producers * 1000)))
echo 'producer utilization '$producer_util'%'
if [ $producer_util -gt 100 ]
then
  echo 'WARNING: producer utilization is too high'
fi
result_dir=${frequency}_${process_time}ms_${n_producers}_${queue_capacity}

NS_LOG=NDNFD:ndn.Consumer:ndn.ProducerThrottled nohup ./waf --run ndnfd-loadbal-csma --command-template="/usr/bin/time %s --n_producers=$n_producers --ns3::ProcessingDelay::QueueCapacity=$queue_capacity --ns3::ProcessingDelay::ProcessTime=${process_time}ms --frequency=$frequency --sim_time=$sim_time --stop_req_time=$stop_req_time" &> ndnfd-loadbal-csma.out
src/NDNFD/examples/ndnfd-loadbal-csma_plot.sh $n_producers $process_time $frequency $sim_time $stop_req_time

mkdir -p $result_dir
mv ndnfd-loadbal-csma* $result_dir/
