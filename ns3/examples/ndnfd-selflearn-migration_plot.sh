#!/bin/bash

grep 'FullDelay' ndnfd-selflearn-migration_delay.tsv > ndnfd-selflearn-migration_delay_FullDelay.tsv
awk '
  BEGIN {
    pps = 5
    frequency = 50
    sim_time = 120
  }
  {
    ++got[int($4/(frequency/pps))]
  }
  END {
    expected = frequency/pps
    for (t=0; t<sim_time*pps; ++t) {
    
      print t/pps, (expected-got[t]) / expected
    }
  }
' ndnfd-selflearn-migration_delay_FullDelay.tsv > ndnfd-selflearn-migration_lost.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-selflearn-migration_plot.pdf";

set xlabel "time(s)";
set xrange [-3:122];
set yrange [0:*];
set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set ylabel "delay(ms)";
plot "ndnfd-selflearn-migration_delay_FullDelay.tsv" using ($1-16):($7/1000) with lines lc 1 title "delay";

set key left top Left reverse samplen 0;

set ylabel "Interest/s";
plot "ndnfd-selflearn-migration_l3.tsv" using ($1-16):2 with lines lc 1 title "mcast send",
     "ndnfd-selflearn-migration_l3.tsv" using ($1-16):3 with lines lc 3 title "mcast recv",
     "ndnfd-selflearn-migration_l3.tsv" using ($1-16):4 with lines lc 7 title "unicast send";

set ylabel "accum Interest";
plot "ndnfd-selflearn-migration_l3.tsv" using ($1-16):5 with lines lc 1 title "mcast send",
     "ndnfd-selflearn-migration_l3.tsv" using ($1-16):6 with lines lc 3 title "mcast recv",
     "ndnfd-selflearn-migration_l3.tsv" using ($1-16):7 with lines lc 7 title "unicast send";

set ylabel "loss (%)";
set yrange [0:100];
plot "ndnfd-selflearn-migration_delay_lost.tsv" using 1:($2*100) with lines lc 1 title "loss";
set yrange [0:*];

'
