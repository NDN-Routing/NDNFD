#!/bin/bash

grep 'FullDelay' ndnfd-multipath_delay.tsv > ndnfd-multipath_fulldelay.tsv
gawk '
  BEGIN {
    pps = 5
    frequency1 = 5
    frequency2 = 95
    sim_time = 90
  }
  $4 < 1000 {
    ++got[int($4/(frequency1/pps))]
  }
  $4 >= 1000 {
    ++got[30*pps+int(($4-1000)/(frequency2/pps))]
  }
  END {
    expected1 = frequency1/pps
    expected2 = frequency2/pps
    for (t=0; t<sim_time*pps; ++t) {
      expected = expected1
      if (t>=30*pps && t<60*pps) {
        expected = expected1 + expected2
      }
      print t/pps, (expected-got[t]) / expected
    }
  }
' ndnfd-multipath_fulldelay.tsv > ndnfd-multipath_loss.tsv

for f in '' E H
do
gawk '
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
' ndnfd-multipath_msgcount$f.tsv > ndnfd-multipath_msgrate$f.tsv
done

gnuplot -e '
set term pdf;
set out "ndnfd-multipath_plot.pdf";

set xlabel "time(s)";
set xrange [0:90];
set border 3;
set key left top Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set ylabel "delay(hop)";
plot "ndnfd-multipath_fulldelay.tsv" using ($1-16):9 with dots lc 1 title "";

set ylabel "delay(ms)";
plot "ndnfd-multipath_fulldelay.tsv" using ($1-16):($7/1000) with dots lc 1 title "";

set ylabel "sent Interests/s";
plot "ndnfd-multipath_msgrateE.tsv" using ($1-16):($2+$5) with lines lc 1 title "upper path",
     "ndnfd-multipath_msgrateH.tsv" using ($1-16):($2+$5) with lines lc 3 title "lower path";

set ylabel "loss (%)";
plot "ndnfd-multipath_loss.tsv" using 1:($2*100) with lines lc 1 title "";
'

