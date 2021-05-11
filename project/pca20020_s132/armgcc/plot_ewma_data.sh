#!/bin/bash
grep "EWMA_DATA" accel_data.log | awk '{print $5}' > ewma_accel.log
gnuplot ewma_plot.gnuplot

