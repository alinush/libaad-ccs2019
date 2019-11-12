#!/bin/bash

run_bench()
{
	size=$1
	batch=$2
	machine=r4.16xlarge
    	day=saturdayeve
    	date=`date +%Y-%m-%d`
    	file=$date-aad-append-time-n-$size-b-$batch-$machine-$day

	AADAppendBench /mnt/pp/test $size $batch 1 -1 $file.csv &>$file.log
}

run_bench 8192 512
run_bench 8192 256
run_bench 8192 128
run_bench 8192 64
run_bench 8192 32
