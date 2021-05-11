set datafile separator ','
set autoscale fix
set key outside right center

set title 'Accelerometer Data'
plot "norm_accel.log" using 1 title 'Normalized Data' with lines

pause -1