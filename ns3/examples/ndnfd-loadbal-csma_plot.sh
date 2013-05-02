#!/bin/bash

n_producers=$1
process_time=$2
frequency=$3
maxseq=$4
sim_time=$5

touch ndnfd-loadbal-csma_lost.tsv

gawk '
  BEGIN {
    maxseq = '$maxseq'
  }
  $5 == "LastDelay" {
    last[$4] = $1-16-$6
    response[$4] = $1-16
  }
  $5 == "FullDelay" {
    first[$4] = $1-16-$6
    retx[$4] = $8
  }
  END {
    maxlast = 0
    for (i=0; i<maxseq; ++i) {
      if (i in first) {
        print i, retx[i], first[i], last[i], response[i] > "ndnfd-loadbal-csma_time.tsv"
        if (last[i] > maxlast) {
          maxlast = last[i]
        }
      } else {
        for (off=1; ; ++off) {
          if (i-off in first) {
            t = first[i-off]
            break
          }
          if (i+off in first) {
            t = first[i+off]
            break
          }
        }
        print i, t > "ndnfd-loadbal-csma_lost.tsv"
      }
    }
    print int(maxlast*1000) > "ndnfd-loadbal-csma_finish.tsv"
  }
' ndnfd-loadbal-csma_delay.tsv

gawk '
  BEGIN {
    maxseq = '$maxseq'
    n_producers = '$n_producers'
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
    for (i=0; i<maxseq; ++i) {
      r = int(responds[i])
      print i, r > "ndnfd-loadbal-csma_serve.tsv"
      ++c[r]
    }
    for (r=0; r<=n_producers; ++r) {
      print r, c[r] > "ndnfd-loadbal-csma_serveh.tsv"
    }
  }
  ' ndnfd-loadbal-csma.out

finish=`cat ndnfd-loadbal-csma_finish.tsv`
n_lost=`wc -l < ndnfd-loadbal-csma_lost.tsv`
avg_retx=`awk '{ sum += $2; ++count } END { print int(sum*1000/count)/1000 }' ndnfd-loadbal-csma_time.tsv`
avg_delay=`awk '{ sum += ($5-$4); ++count } END { print int(sum*1000/count) "ms" }' ndnfd-loadbal-csma_time.tsv`

plot='
set term pdf;
set out "ndnfd-loadbal-csma_plot.pdf";

set xlabel "sequence number";
set xrange [0:'$maxseq'];
set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;
set boxwidth 0.6 relative;
set style fill solid 1.0;

set title "progress";
set ylabel "time(s)";
set yrange [0:*];
set border 11;
set y2tics border nomirror in;
set y2label "retransmissions";
set y2range [1:*];
set logscale y2;
plot "ndnfd-loadbal-csma_time.tsv" using 1:3:(0):(0+$4-$3) with vectors nohead lc 4 lw 1 title "first Interest",
     "ndnfd-loadbal-csma_time.tsv" using 1:4:(0):(0+$5-$4) with vectors nohead lc 3 lw 1 title "last Interest",
     "ndnfd-loadbal-csma_lost.tsv" using 1:2 with dots lc 1 title "lost('$n_lost')",
     "ndnfd-loadbal-csma_time.tsv" using 1:2 axes x1y2 with dots lc 8 title "retx('$avg_retx')",
     "ndnfd-loadbal-csma_finish.tsv" using (0):($1/1000):('$maxseq'):(0) with vectors nohead lc 1 lw 1 title "finish('$finish'ms)";
set border 3;
unset y2tics;
unset y2label;

set title "delay since last retransmission";
set ylabel "delay(ms)";
set yrange [0:*];
plot "ndnfd-loadbal-csma_time.tsv" using 1:((0+$5-$4)*1000) with dots lc 3 lw 1 title "delay('$avg_delay')",
     "ndnfd-loadbal-csma_lost.tsv" using 1:(2000) with dots lc 1 title "lost('$n_lost')";

set title "number of producers used per segment";
set ylabel "used producers";
set yrange [0:'$n_producers'];
plot "ndnfd-loadbal-csma_serve.tsv" using 1:2 with dots lc 3 title "";

set title "number of producers used";
set xrange [-0.5:'$n_producers'+0.5];
set ylabel "number of segments";
set yrange [0:*];
plot "ndnfd-loadbal-csma_serveh.tsv" using 1:2 with boxes lc 3 title "",
     "ndnfd-loadbal-csma_serveh.tsv" using 1:2:2 with labels lc 3 title "";
'
gnuplot -e "$plot"

