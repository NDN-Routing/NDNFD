#!/bin/bash

frequency=2
sim_time=20

grep 'FullDelay' ndnfd-fattree_delay.tsv > ndnfd-fattree_delay_FullDelay.tsv
gawk '
  {
    frequency = '$frequency'
    ++got[$4 % 10000]
  }
  END {
    for (i=0;i<='$sim_time'*frequency;++i) {
      lost[int(i/frequency)] += 16*16 - got[i]
    }
    for (t=0;t<'$sim_time';++t) {
      print t, int(lost[t])
    }
  }
' ndnfd-fattree_delay_FullDelay.tsv > ndnfd-fattree_lost.tsv

gawk '
  NR == 1 {
    for (c=2; c<=NF; ++c) {
      last[c] = 0
    }
  }
  NR > 1 {
    $1 = $1-1
    for (c=2; c<=NF; ++c) {
      diff[c] = $c - last[c]
      last[c] = $c
      $c = diff[c]
    }
    print
  }
' ndnfd-fattree_msgcount.tsv > ndnfd-fattree_msgrate.tsv

gawk '
  $0 ~ "SelfLearnStrategy::StartFlood|AslStrategy::Flood" {
    ++flood[int($1-16)]
  }
  END {
    for (t=0;t<'$sim_time';++t) {
      print t, int(flood[t])
    }
  }
' ndnfd-fattree.out > ndnfd-fattree_flood.tsv

gawk '
  function distance(x,y) {
    x = int(x)
    y = int(y)
    if (int(x/10) == int(y/10)) {
      if (x==y) return 0
      if (x%10 <= 1 && y%10 <= 1) return 2
      if (x%10 >= 2 && y%10 >= 2) return 2
      return 4
    }
    return 6
  }
  {
    consumer = int($4/1000000)
    producer = int($4/10000)%100
    seq = $4 % 10000
    dist = distance(consumer,producer)
    hops = $9
    print consumer, producer, seq, dist, hops
  }
' ndnfd-fattree_delay_FullDelay.tsv > ndnfd-fattree_pathlen.tsv
gawk '
  BEGIN {
    n_consumers = 16*(16-1)
    maxseq = 0
  }
  $5 > 0 {
    if (maxseq < $3) { maxseq = $3 }
    dist[$1 " " $2] = $4
    len = $5
    len += len%2
    if (len > 8) len = 8
    ++c[$3 " " len]
  }
  function print_c(prefix,file) {
    NF = 6
    $1 = prefix
    $3 = int(c[prefix " 2"])
    $4 = int(c[prefix " 4"])
    $5 = int(c[prefix " 6"])
    $6 = int(c[prefix " 8"])
    $2 = n_consumers-$3-$4-$5-$6
    print > file
  }
  END {
    for (consumer_producer in dist) {
      ++c["dist " dist[consumer_producer]]
    }
    print_c("dist","ndnfd-fattree_pathlen-distance.tsv")
    for (seq=0; seq<=maxseq; ++seq) {
      print_c(seq,"ndnfd-fattree_pathlen-actual.tsv")
    }
  }
' ndnfd-fattree_pathlen.tsv


gnuplot -e '
set term pdf;
set out "ndnfd-fattree_plot.pdf";

set xlabel "time(s)";
set xrange [0:'$sim_time'];
set yrange [0:];
set y2range [0:];
set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set title "delay and lost";
set border 11;
set ylabel "delay(ms)";
set y2tics border nomirror in;
set y2label "loss/s";
plot "ndnfd-fattree_delay_FullDelay.tsv" using ($1-$6-16):($7/1000) with dots lc 3 title "delay",
     "ndnfd-fattree_lost.tsv" using 1:2 axes x1y2 with lines lc 1 title "loss";

set title "Interest send and flood";
set ylabel "Interest/s";
set y2tics border nomirror in;
set y2label "flood/s";
plot "ndnfd-fattree_msgrate.tsv" using ($1-16):($2+$5) with lines lc 1 title "send",
     "ndnfd-fattree_flood.tsv" using 1:2 axes x1y2 with lines lc 3 title "flood";
unset y2tics;
unset y2label;

set title "path inflation";
set key invert reverse Left outside;
set xrange [0:40];
set yrange [0:256];
set boxwidth 0.6 relative;
set xlabel "sequence number";
set ylabel "consumer,producer pairs";
plot "ndnfd-fattree_pathlen-actual.tsv" using 1:($3) with lines lc 1 lw 2 title "≤ 2 hops",
     "ndnfd-fattree_pathlen-actual.tsv" using 1:($3+$4) with lines lc 3 lw 2 title "≤ 4 hops",
     "ndnfd-fattree_pathlen-actual.tsv" using 1:($3+$4+$5) with lines lc 4 lw 2 title "≤ 6 hops",
     "ndnfd-fattree_pathlen-distance.tsv" using (0):($3):(40):(0) with vectors nohead lc 1 lt 0 lw 6 title "",
     "ndnfd-fattree_pathlen-distance.tsv" using (0):($3+$4):(40):(0) with vectors nohead lc 3 lt 0 lw 6 title "",
     "ndnfd-fattree_pathlen-distance.tsv" using (0):($3+$4+$5):(40):(0) with vectors nohead lc 4 lt 0 lw 6 title "",
     "" using (0):(0) with lines lc 7 lw 2 title "actual path",
     "" using (0):(0) with lines lc 7 lt 0 lw 6 title "shortest path";
'
