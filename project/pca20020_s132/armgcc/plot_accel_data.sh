#!/bin/bash
grep "ACCEL_DATA" accel_data.log | awk '{print $5}' > raw_accel.log
gnuplot plot_accel_data.gnuplot

