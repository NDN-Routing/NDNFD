#!/bin/bash

n_producers=4
process_time=10
frequency=300
sim_time=10
stop_req_time=$(($sim_time-1))

NS_LOG=NDNFD:ndn.Consumer:ndn.ProducerThrottled nohup ./waf --run ndnfd-loadbal-csma --command-template="/usr/bin/time %s --n_producers=$n_producers --process_time=$process_time --frequency=$frequency --sim_time=$sim_time --stop_req_time=$stop_req_time" &> ndnfd-loadbal-csma.out
src/NDNFD/examples/ndnfd-loadbal-csma_plot.sh $n_producers $process_time $frequency $sim_time $stop_req_time

