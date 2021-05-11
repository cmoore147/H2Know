set datafile separator ','
set autoscale fix
set key outside right center

set title 'Accelerometer Data'
plot "ewma_accel.log" using 1 title 'EWMA-filtered Data' with lines

pause -1