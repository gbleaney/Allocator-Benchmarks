// Debugging Settings
#define DEBUG
//#define DEBUG_V1
//#define DEBUG_V2
//#define DEBUG_V3
//#define DEBUG_V4

#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>

#include "benchmark_common.h"

// Debugging
#include <typeinfo>
#include <assert.h>

alignas(long long) static char pool[1ull << 30];

template<typename ALLOC>
void run_allocation(const unsigned long long N_expanded, const unsigned long long S_expanded, ALLOC alloc) {
	// N_expanded = Number of iterations
	// S_expanded = Chunk size parameter - size in bytes of memory to be allocated
	// alloc = Allocator to be used to allocate memory

#ifdef DEBUG_V3
	std::cout << "Running allocations for " << N_expanded << " chunks of " << S_expanded << " bytes" << std::endl;
#endif // DEBUG_V3
	
	for (unsigned long long i = 0; i < N_expanded; i++)	{
		char * memory = alloc.allocate(S_expanded);
		++(memory[0]);
		escape(memory);
		alloc.deallocate(memory, S_expanded);
	}

}

void run_row(size_t N, size_t S, size_t W) {
	// N = Number of iterations (power of 2)
	// S = Chunk size parameter - size in bytes of memory to be allocated (power of 2)
	// W = Number of active threads
	unsigned long long N_expanded = 1ull << N;
	unsigned long long S_expanded = 1ull << S;

#ifdef DEBUG_V1
	std::cout << "N: 2^" << N << "=" << N_expanded << " S: 2^" << S << "=" << S_expanded << " W: " << W << std::endl;
#endif // DEBUG_V1

	std::cout << "N=^" << N << " S=" << S << " W=" << W << std::flush;

	for (size_t i = 0; i < 8; i++) {

		std::clock_t c_start = std::clock();

		for (size_t thread = 0; thread < W; thread++) {
			int pid = fork();
			if (pid == 0) { // Child process
				switch (i) {
				case 0: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS1" << std::endl;
#endif // DEBUG_V3
					std::allocator<char> alloc;
					run_allocation<std::allocator<char>>(N_expanded, S_expanded, alloc);
					break;
				}
				case 1: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS2" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bslma::NewDeleteAllocator alloc;
					run_allocation<typename alloc_adaptors<char>::polymorphic>(N_expanded, S_expanded, &alloc);
					break;
				}
				case 2: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS3" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
					run_allocation<typename alloc_adaptors<char>::monotonic>(N_expanded, S_expanded, &alloc);
					break;
				}
				case 3: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS5" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
					run_allocation<typename alloc_adaptors<char>::polymorphic>(N_expanded, S_expanded, &alloc);
					break;
				}
				case 4: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS7" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bdlma::MultipoolAllocator alloc;
					run_allocation<typename alloc_adaptors<char>::multipool>(N_expanded, S_expanded, &alloc);
					break;
				}
				case 5: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS9" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bdlma::MultipoolAllocator alloc;
					run_allocation<typename alloc_adaptors<char>::polymorphic>(N_expanded, S_expanded, &alloc);
					break;
				}
				case 6: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS11" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
					BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
					run_allocation<typename alloc_adaptors<char>::multipool>(N_expanded, S_expanded, &alloc);
					break;
				}
				case 7: {
#ifdef DEBUG_V3
					std::cout << std::endl << "AS13" << std::endl;
#endif // DEBUG_V3
					BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
					BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
					run_allocation<typename alloc_adaptors<char>::polymorphic>(N_expanded, S_expanded, &alloc);
					break;
				}
				default:
					break;
				}
				exit(0);
			}
		}

		int error;
		while (wait(&error) > 0) {
#ifdef DEBUG_V1
			std::cout << "Finished waiting on process" << std::endl;
#endif // DEBUG_V1
			if (error) {
				std::cout << "One or more threads failed. Error: " << error << std::endl;
			}
		}

		std::clock_t c_end = std::clock();
		std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " " << std::flush;
	}
	std::cout << std::endl;
}

void run_table(size_t N, size_t S) {
	std::cout << std::endl << "Running table of 2^" << N << " (N) iterations and 2^" << S << " (S) bytes." << std::endl;
	for (size_t W = 1; W <= 8; W++)
	{
		run_row(N, S, W);
	}
}

int main(int argc, char *argv[]) {
	//TODO:
	// 1) Providing pre-allocated pool to monotonic - best size? Does it even make sense?
	// 2) Regardless of multithreading, we are biasing heavily toward multipool becaue of it's use of the free list.
	//    Looking at the paper, as we add more threads, the gap between multipool and default closes, implying that 
	//    
	std::cout << "Started" << std::endl;
	
	run_table(15, 6);
	run_table(15, 7);
	run_table(15, 8);
	run_table(16, 8);
	run_table(17, 8);
	run_table(18, 8);
	run_table(19, 8);

	std::cout << "Done" << std::endl;
}
