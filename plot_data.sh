#!/bin/bash
grep "EWMA_DATA" accel_data.log | awk '{print $5}' > ewma_accel.log
grep "ACCEL_DATA" accel_data.log | awk '{print $5}' > raw_accel.log
grep "NORM_DATA" accel_data.log | awk '{print $5}' > norm_accel.log