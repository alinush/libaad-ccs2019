#pragma once

#include <vector>
#include <iostream>
#include <iomanip>
#include <chrono>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>

#include <xutils/Log.h>

using std::endl;
using std::cout;
using namespace libfqfft;

/**
 * Originally from: https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
 *
 * process_mem_usage(double &, double &) - takes two doubles by reference,
 * attempts to read the system-dependent data for a process' virtual memory
 * size and resident set size, and return the results in bytes.
 *
 * On failure, returns 0, 0;
 */
void printMemUsage(const char * headerMessage = nullptr);
void getMemUsage(size_t& vm_usage, size_t& resident_set);

void printOpPerf(const std::chrono::microseconds::rep& usecs, const char * operation, size_t inputSize);
