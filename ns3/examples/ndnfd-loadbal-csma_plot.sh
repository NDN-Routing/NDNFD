#!/bin/bash

n_producers=$1
process_time=$2
n_consumers=$3
maxseq=$4
sim_time=$5

touch ndnfd-loadbal-csma_lost.tsv

gawk '
  BEGIN {
    maxseq = '$maxseq'
    n_consumers = '$n_consumers'
  }
  $5 == "LastDelay" {
    consumer = int($4/1000000)
    i = $4
    last_t = $1-16-$6
    if (last_t > finish[consumer]) {
      finish[consumer] = last_t
    }
    last[i] = last_t
    response[i] = $1-16
  }
  $5 == "FullDelay" {
    i = $4
    first[i] = $1-16-$6
    retx[i] = $8
  }
  END {
    for (s=0; s<maxseq; ++s) {
      for (i=s; i<1000000*n_consumers; i+=1000000) {
        t = -1
        if (i in first) {
          t = first[i]
          print i, retx[i], first[i], last[i], response[i] > "ndnfd-loadbal-csma_time.tsv"
        } else {
          for (off=1; off<maxseq; ++off) {
            if (i-off in first) {
              t = first[i-off]
              break
            }
            if (i+off in first) {
              t = first[i+off]
              break
            }
          }
          if (t >= 0) {
            print i, t > "ndnfd-loadbal-csma_lost.tsv"
          }
        }
      }
    }
    for (consumer=0; consumer<n_consumers; ++consumer) {
      print consumer, int(finish[consumer]*1000) > "ndnfd-loadbal-csma_finish.tsv"
    }
  }
' ndnfd-loadbal-csma_delay.tsv

gawk '
  BEGIN {
    maxseq = '$maxseq'
    n_producers = '$n_producers'
    n_consumers = '$n_consumers'
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
    for (s=0; s<maxseq; ++s) {
      for (i=s; i<1000000*n_consumers; i+=1000000) {
        r = int(responds[i])
        print i, r > "ndnfd-loadbal-csma_serve.tsv"
        ++c[r]
      }
    }
    for (r=0; r<=n_producers; ++r) {
      print r, int(c[r]) > "ndnfd-loadbal-csma_serveh.tsv"
    }
  }
  ' ndnfd-loadbal-csma.out

sed -n '/Window: / s/^\([0-9\.]*\)s .* Window: \([0-9]*\), InFlight: \([0-9]*\)$/\1\t\2\t\3/ p' ndnfd-loadbal-csma.out >  ndnfd-loadbal-csma_window.tsv

finish=`cut -d' ' -f 2-2 ndnfd-loadbal-csma_finish.tsv | sort -n | tail -1`
n_lost=`wc -l < ndnfd-loadbal-csma_lost.tsv`
avg_retx=`awk '{ sum += $2; ++count } END { print int(sum*1000/count)/1000 }' ndnfd-loadbal-csma_time.tsv`
avg_delay=`awk '{ sum += ($5-$4); ++count } END { print int(sum*1000/count) "ms" }' ndnfd-loadbal-csma_time.tsv`

plot='
set term pdf;
set out "ndnfd-loadbal-csma_plot.pdf";

set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;
set boxwidth 0.6 relative;
set style fill solid 1.0;

set title "finish time";
set xlabel "consumer";
set xrange [-0.5:'$n_consumers'.5];
set ylabel "time(s)";
set yrange [0:*];
plot "ndnfd-loadbal-csma_finish.tsv" using 1:($2/1000) with boxes lc 3 title "",
     "ndnfd-loadbal-csma_finish.tsv" using 1:($2/1000):(sprintf("%d",$2/1000)) with labels lc 3 title "";

set title "progress";
set xlabel "sequence number";
set xrange [0:'$maxseq'];
set ylabel "time(s)";
set yrange [0:*];
set border 11;
set y2tics border nomirror in;
set y2label "retransmissions";
set y2range [1:*];
set logscale y2;
plot "ndnfd-loadbal-csma_time.tsv" using (int($1)%1000000):3:(0):(0+$4-$3) with vectors nohead lc 4 lw 1 title "first Interest",
     "ndnfd-loadbal-csma_time.tsv" using (int($1)%1000000):4:(0):(0+$5-$4) with vectors nohead lc 3 lw 1 title "last Interest",
     "ndnfd-loadbal-csma_lost.tsv" using (int($1)%1000000):2 with dots lc 1 title "lost('$n_lost')",
     "ndnfd-loadbal-csma_time.tsv" using (int($1)%1000000):2 axes x1y2 with dots lc 8 title "retx('$avg_retx')";
set border 3;
unset y2tics;
unset y2label;

set title "delay since last retransmission";
set ylabel "delay(ms)";
set yrange [0:*];
plot "ndnfd-loadbal-csma_time.tsv" using (int($1)%1000000):((0+$5-$4)*1000) with dots lc 3 lw 1 title "delay('$avg_delay')",
     "ndnfd-loadbal-csma_lost.tsv" using (int($1)%1000000):(2000) with dots lc 1 title "lost('$n_lost')";

set title "producer usage";
set ylabel "number of producer used";
set yrange [0:'$n_producers'];
plot "ndnfd-loadbal-csma_serve.tsv" using (int($1)%1000000):2 with dots lc 3 title "";

set title "producer usage";
set xlabel "number of producers";
set xrange [-0.5:'$n_producers'+0.5];
set ylabel "number of segments";
set yrange [0:*];
plot "ndnfd-loadbal-csma_serveh.tsv" using 1:2 with boxes lc 3 title "",
     "ndnfd-loadbal-csma_serveh.tsv" using 1:2:2 with labels lc 3 title "";

set title "window size by time";
set xlabel "time(s)";
set xrange [0:*];
set ylabel "number of Interests";
set yrange [0:*];
plot "ndnfd-loadbal-csma_window.tsv" using ($1-16):2 with lines lc 1 title "window";
'
gnuplot -e "$plot"

