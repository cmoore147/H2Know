#!/bin/bash
grep "NORM_DATA" accel_data.log | awk '{print $5}' > norm_accel.log
gnuplot normalized_plot.gnuplot

