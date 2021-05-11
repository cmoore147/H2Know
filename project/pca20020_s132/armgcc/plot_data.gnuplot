set datafile separator ','
#set autoscale fix

set multiplot layout 5,1 rowsfirst

plot "raw_accel.log" using 1 title 'X-axis' with lines
plot "raw_accel.log" using 2 title 'Y-axis' with lines
plot "raw_accel.log" using 3 title 'Z-axis' with lines
plot "norm_accel.log" using 1 title 'Normalized Accel Data' with lines
plot "ewma_accel.log" using 1 title 'EWMA Accel Data' with lines


pause -1