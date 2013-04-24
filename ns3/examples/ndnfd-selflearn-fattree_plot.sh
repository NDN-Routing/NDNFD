#!/bin/bash

grep 'FullDelay' ndnfd-selflearn-fattree_delay.tsv > ndnfd-selflearn-fattree_delay_FullDelay.tsv
awk '
  {
    ++got[$4 % 1000000];
  }
  END {
    for (i=0;i<=20*2;++i) {
      lost[int(i/2)] += 16*16 - got[i]
    }
    for (t=0;t<20;++t) {
      if (lost[t]) {
        print t "\t" lost[t];
      }
    }
  }
  ' ndnfd-selflearn-fattree_delay_FullDelay.tsv > ndnfd-selflearn-fattree_lost.tsv
awk '
  $0 ~ "SelfLearnStrategy::StartFlood" {
    ++flood[int($1)]
  }
  END {
    for (t=0;t<20;++t) {
      if (flood[t]) {
        print t "\t" flood[t];
      } else {
        print t "\t" 0;
      }
    }
  }
  ' nohup.out > ndnfd-selflearn-fattree_flood.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-selflearn-fattree_plot.pdf";

set xlabel "time(s)";
set xrange [0:20];
set yrange [0:];
set y2range [0:];
set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set border 11;
set ylabel "delay(ms)";
set y2tics border nomirror in;
set y2label "loss/s";
plot "ndnfd-selflearn-fattree_delay_FullDelay.tsv" using ($1-$6-16):($7/1000) with points lc 3 pt 0 title "delay",
     "ndnfd-selflearn-fattree_lost.tsv" using 1:2 axes x1y2 with points lc 1 pt 2 ps 0.4 title "loss";

set ylabel "Interest/s";
set y2tics border nomirror in;
set y2label "flood/s";
plot "ndnfd-selflearn-fattree_l3.tsv" using ($1-16):($2+$4) with lines lc 1 title "send",
     "ndnfd-selflearn-fattree_flood.tsv" using ($1-16):2 axes x1y2 with lines lc 3 title "flood"
'
