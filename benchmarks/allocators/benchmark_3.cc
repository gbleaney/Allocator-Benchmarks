// Debugging Settings
#define DEBUG
//#define DEBUG_V1
//#define DEBUG_V2
//#define DEBUG_V3
//#define DEBUG_V4

#include <iostream>

#include "benchmark_common.h"

// Debugging
#include <typeinfo>
#include <assert.h>

void run_row(size_t T, size_t A, size_t S) {
	// T = Total amount of memory to be allocated (power of 2)
	// A = Active memory - maximum amount of memory to be allocated at a time (power of 2)
	// S = Size in bytes of chunk to be allocated (power of 2)
	size_t expanded_T = 1 << T;
	size_t expanded_A = 1 << A;
	size_t expanded_S = 1 << S;

	size_t chunks = 1 << (A - T);

	std::cout << "T: 2^" << T << "=" << expanded_T << " A: 2^" << A << "=" << expanded_A << " S: 2^" << S << "=" << expanded_S << " Chunks: " << chunks << std::endl;


}

int main(int argc, char *argv[]) {
	std::cout << "Started" << std::endl;
	
	run_row(35, 20, 15);

	std::cout << "Done" << std::endl;
}
