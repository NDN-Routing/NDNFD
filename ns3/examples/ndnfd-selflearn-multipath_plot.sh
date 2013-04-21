#!/bin/bash

grep 'FullDelay' ndnfd-selflearn-multipath_delay.tsv > ndnfd-selflearn-multipath_delay_FullDelay.tsv
awk '
  {
    got[$4] = 1;
  }
  END {
    for (i=0; i<=90*5; ++i) {
      t = i/5;
      if (! got[i]) {
        ++lost[int(t)]
      }
    }
    for (i=1000000000; i<1000000000+45*30; ++i) {
      t = 30 + (i-1000000000)/45;
      if (! got[i]) {
        ++lost[int(t)]
      }
    }
    for (t=0;t<90;++t) {
      if (lost[t]) {
        print t "\t" lost[t];
      }
    }
  }
  ' ndnfd-selflearn-multipath_delay_FullDelay.tsv > ndnfd-selflearn-multipath_lost.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-selflearn-multipath_plot.pdf";

set xlabel "time(s)";
set xrange [0:90];
set border 3;
set key left top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set border 11;
set ylabel "delay(ms)";
set y2tics border nomirror in;
set y2label "loss/s";
plot "ndnfd-selflearn-multipath_delay_FullDelay.tsv" using ($1-$6-16):($7/1000) with points lc 3 pt 0 title "delay",
     "ndnfd-selflearn-multipath_lost.tsv" using 1:2 axes x1y2 with points lc 1 pt 2 ps 0.4 title "loss";
set border 3;
unset y2tics;
unset y2label;

set ylabel "Interest/s";
plot "ndnfd-selflearn-multipath_l3_E.tsv" using ($1-16):($2+$4) with lines lc 1 title "upper path",
     "ndnfd-selflearn-multipath_l3_H.tsv" using ($1-16):($2+$4) with lines lc 3 title "lower path";

plot "ndnfd-selflearn-multipath_l3.tsv" using ($1-16):2 with lines lc 1 title "mcast",
     "ndnfd-selflearn-multipath_l3.tsv" using ($1-16):4 with lines lc 7 title "unicast";
'
