set datafile separator ','
set autoscale fix
set key outside right center

set title 'Accelerometer Data'
plot "raw_accel.log" using 1 title 'X-axis' with lines
replot "raw_accel.log" using 2 title 'Y-axis' with lines
replot "raw_accel.log" using 3 title 'Z-axis' with lines

pause -1