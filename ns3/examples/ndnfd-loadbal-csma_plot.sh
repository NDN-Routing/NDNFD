#!/bin/bash

n_producers=4
process_time=10
frequency=300
sim_time=10
stop_req_time=$(($sim_time-1))

gawk '
  BEGIN {
    pps = 5
    frequency = '$frequency' /pps
    time = '$sim_time' *pps
    stop_req_time = '$stop_req_time' *pps
    n_producers = '$n_producers'
  }
  $3 ~ "ndn.ProducerThrottled:ProcessComplete" {
    producer = $2
    getline
    if (substr($2,1,8)=="/prefix/") {
      i = substr($2,9)
      ++responds[i]
      ++usage[int(i/frequency) "," producer]
    }
  }
  END {
    for (t=0;t<time;++t) {
      unhandled = 0
      if (t<stop_req_time) {
        unhandled = frequency
        for (i=t*frequency;i<(t+1)*frequency;++i) {
          if (responds[i]) --unhandled
        }
      }
      row = t/pps "\t" unhandled*pps
      for (p=1;p<=n_producers;++p) {
        if (usage[t "," p]) {
          row = row "\t" usage[t "," p]*pps
        } else {
          row = row "\t0"
        }
      }
      print row
    }
  }
  ' nohup.out > ndnfd-loadbal-csma_producer-use.tsv

grep 'FullDelay' ndnfd-loadbal-csma_delay.tsv > ndnfd-loadbal-csma_delay_FullDelay.tsv
gawk '
  BEGIN {
    pps = 5
    frequency = '$frequency' /pps
    stop_req_time = '$stop_req_time' *pps
  }
  {
    ++got[$4]
  }
  END {
    for (t=0;t<stop_req_time;++t) {
      lost = 0
      for (i=t*frequency;i<(t+1)*frequency;++i) {
        if (!got[i]) { ++lost }
      }
      print t/pps "\t" lost/frequency
    }
  }
  ' ndnfd-loadbal-csma_delay_FullDelay.tsv > ndnfd-loadbal-csma_lost.tsv

gawk '
  BEGIN {
    frequency = '$frequency'
    stop_req_time = '$stop_req_time'
  }
  $3 ~ "ndn.ProducerThrottled:ProcessComplete" {
    producer = $2
    getline
    if (substr($2,1,8)=="/prefix/") {
      i = substr($2,9)
      ++responds[i]
    }
  }
  END {
    for (i=0;i<stop_req_time*frequency;++i) {
      if (responds[i]) {
        print i "\t" i/frequency "\t" responds[i]
      } else {
        print i "\t" i/frequency "\t" 0
      }
    }
  }
  ' nohup.out > ndnfd-loadbal-csma_serve.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-loadbal-csma_plot.pdf";

set xlabel "time(s)";
set xrange [0:10];
set yrange [0:];
set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set ylabel "requests/s";
plot "ndnfd-loadbal-csma_producer-use.tsv" using 1:2 with lines lc 1 lw 4 title "unhandled",
     "ndnfd-loadbal-csma_producer-use.tsv" using 1:3 with lines lc 2 title "producer1",
     "ndnfd-loadbal-csma_producer-use.tsv" using 1:4 with lines lc 3 title "producer2",
     "ndnfd-loadbal-csma_producer-use.tsv" using 1:5 with lines lc 4 title "producer3",
     "ndnfd-loadbal-csma_producer-use.tsv" using 1:6 with lines lc 5 title "producer4";

set ylabel "delay(ms)";
set y2tics border nomirror in;
set y2label "loss/s";
set y2range [0:100];
set border 11;
plot "ndnfd-loadbal-csma_delay_FullDelay.tsv" using ($1-$6-16):($7/1000) with points lc 3 pt 0 title "delay",
     "ndnfd-loadbal-csma_lost.tsv" using 1:($2*100) axes x1y2 with lines lc 1 title "loss%";
unset y2tics;
unset y2label;
set border 3;

set ylabel "services";
plot "ndnfd-loadbal-csma_serve.tsv" using 2:3 with points lc 1 pt 0 title "";
'

