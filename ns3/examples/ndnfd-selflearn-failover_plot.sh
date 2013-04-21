#!/bin/bash

grep 'FullDelay' ndnfd-selflearn-failover_delay.tsv > ndnfd-selflearn-failover_delay_FullDelay.tsv

gnuplot -e '
set term pdf;
set out "ndnfd-selflearn-failover_plot.pdf";

set xlabel "time(s)";
set border 3;
set key right bottom Left reverse samplen 0;
set xtics nomirror;
set ytics nomirror;

set ylabel "delay(ms)";
plot "ndnfd-selflearn-failover_delay_FullDelay.tsv" using ($1-16):($7/1000) with points lc 1 pt 0 title "delay";

set ylabel "Interest/s";
plot "ndnfd-selflearn-failover_l3.tsv" using ($1-16):2 with lines lc 1 title "mcast send",
     "ndnfd-selflearn-failover_l3.tsv" using ($1-16):3 with lines lc 3 title "mcast recv",
     "ndnfd-selflearn-failover_l3.tsv" using ($1-16):4 with lines lc 7 title "unicast send";

set ylabel "accum Interest";
plot "ndnfd-selflearn-failover_l3.tsv" using ($1-16):5 with lines lc 1 title "mcast send",
     "ndnfd-selflearn-failover_l3.tsv" using ($1-16):6 with lines lc 3 title "mcast recv",
     "ndnfd-selflearn-failover_l3.tsv" using ($1-16):7 with lines lc 7 title "unicast send";
'
