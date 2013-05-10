#!/bin/bash

frequency=2
sim_time=20

grep 'FullDelay' ndnfd-selflearn-fattree_delay.tsv > ndnfd-selflearn-fattree_delay_FullDelay.tsv
awk '
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
' ndnfd-selflearn-fattree_delay_FullDelay.tsv > ndnfd-selflearn-fattree_lost.tsv

awk '
  $0 ~ "SelfLearnStrategy::StartFlood" {
    ++flood[int($1-16)]
  }
  END {
    for (t=0;t<'$sim_time';++t) {
      print t, int(flood[t])
    }
  }
' ndnfd-selflearn-fattree.out > ndnfd-selflearn-fattree_flood.tsv

awk '
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
  $0 ~ "SelfLearnStrategy::DidSatisfyPendingInterests" {
    if (match($0,/SelfLearnStrategy::DidSatisfyPendingInterests\(\/H[0-3][0-3]\/([0-9]*)\).*matching\_suffix=0/,m)) {
      seqnum = m[1]
      ++satisfy[seqnum]
    }
  }
  END {
    for (seqnum in satisfy) {
      consumer = int(seqnum/1000000)
      producer = int(seqnum/10000)%100
      seq = seqnum % 10000
      dist = distance(consumer,producer)
      print consumer, producer, seq, dist, satisfy[seqnum]
    }
  }
' ndnfd-selflearn-fattree.out > ndnfd-selflearn-fattree_pathlen.tsv
awk '
  {
    infla = $5-1-$4
    ++c[$3 "\t" infla]
    a[$3] += infla
    ++n[$3]
    if (infla == 0) {
      ++b[$3]
    }
  }
  END {
    for (seq_infla in c) {
      print seq_infla, c[seq_infla] > "ndnfd-selflearn-fattree_path-infla.tsv"
    }
    for (seq in n) {
      print seq, a[seq] / n[seq] > "ndnfd-selflearn-fattree_path-infla-avg.tsv"
    }
    for (seq in n) {
      print seq, int(b[seq]) / n[seq] > "ndnfd-selflearn-fattree_path-infla-zero.tsv"
    }
  }
' ndnfd-selflearn-fattree_pathlen.tsv

sort -n -k1,2 ndnfd-selflearn-fattree_path-infla.tsv -o ndnfd-selflearn-fattree_path-infla.tsv
sort -n -k1 ndnfd-selflearn-fattree_path-infla-avg.tsv -o ndnfd-selflearn-fattree_path-infla-avg.tsv
sort -n -k1 ndnfd-selflearn-fattree_path-infla-zero.tsv -o ndnfd-selflearn-fattree_path-infla-zero.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-selflearn-fattree_plot.pdf";

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
plot "ndnfd-selflearn-fattree_delay_FullDelay.tsv" using ($1-$6-16):($7/1000) with dots lc 3 title "delay",
     "ndnfd-selflearn-fattree_lost.tsv" using 1:2 axes x1y2 with lines lc 1 title "loss";

set title "Interest send and flood";
set ylabel "Interest/s";
set y2tics border nomirror in;
set y2label "flood/s";
plot "ndnfd-selflearn-fattree_l3.tsv" using ($1-16):($2+$4) with lines lc 1 title "send",
     "ndnfd-selflearn-fattree_flood.tsv" using 1:2 axes x1y2 with lines lc 3 title "flood";

set title "path inflation";
set ylabel "extra path length (hop)";
set y2tics border nomirror in;
set y2label "shortest path usage (%)";
set y2range [0:100];
set key right center Left reverse samplen 0;
set style fill transparent solid 1 noborder;
plot "ndnfd-selflearn-fattree_path-infla.tsv" using ($1/'$frequency'):2:(log10($3)/12) with circles lc 1 title "path inflation",
     "ndnfd-selflearn-fattree_path-infla-zero.tsv" using ($1/'$frequency'):($2*100) axes x1y2 with lines lc 3 title "using shortest path";

'
