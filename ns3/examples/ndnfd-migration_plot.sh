#!/bin/bash

strategy=$1

grep 'FullDelay' ndnfd-migration_delay.tsv > ndnfd-migration_fulldelay.tsv
gawk '
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
' ndnfd-migration_fulldelay.tsv > ndnfd-migration_loss.tsv

awk '
  NR == 1 {
    for (c=2; c<=NF; ++c) {
      last[c] = 0
    }
  }
  NR > 1 {
    for (c=2; c<=NF; ++c) {
      diff[c] = $c - last[c]
      last[c] = $c
      $c = diff[c]
    }
    print
  }
' ndnfd-migration_msgcount.tsv > ndnfd-migration_msgrate.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-migration_plot.pdf";

set xlabel "time(s)";
set xrange [-3:122];
set yrange [0:*];
set border 3;
set key right top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set ylabel "delay(hop)";
plot "ndnfd-migration_fulldelay.tsv" using ($1-16):9 with lines lc 1 title "";

set ylabel "delay(ms)";
plot "ndnfd-migration_fulldelay.tsv" using ($1-16):($7/1000) with lines lc 1 title "";

set ylabel "sent Interests/s";
plot "ndnfd-migration_msgrate.tsv" using ($1-16):2 with lines lc 1 title "mcast",
     "ndnfd-migration_msgrate.tsv" using ($1-16):5 with lines lc 3 title "unicast";

set ylabel "processed messages/s";
plot "ndnfd-migration_msgrate.tsv" using ($1-16):($8+$11) with lines lc 1 title "Interest",
     "ndnfd-migration_msgrate.tsv" using ($1-16):($9+$12) with lines lc 3 title "ContentObject",
     "ndnfd-migration_msgrate.tsv" using ($1-16):($10+$13) with lines lc 7 title "Nack";

set ylabel "loss (%)";
plot "ndnfd-migration_loss.tsv" using 1:($2*100) with lines lc 1 title "";

'
