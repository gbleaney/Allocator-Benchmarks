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

alignas(long long) static char pool[1ull << 30];

template<typename ALLOC>
void run_allocation(const unsigned long long expanded_T, const unsigned long long expanded_A, const unsigned long long expanded_S, ALLOC alloc) {
	// expanded_T = Total amount of memory to be allocated
	// expanded_A = Active memory. The maximum amount of memory to be allocated at a time
	// expanded_S = Size in bytes of chunk to be allocated
	// alloc = Allocator to be used to allocate memory

	std::vector<char *> allocated;
	allocated.reserve(expanded_A);

	escape(allocated.data());

	std::clock_t c_start = std::clock();
	
	for (unsigned long long i = 0; i < expanded_A; i++)	{
		allocated.emplace_back(alloc.allocate(expanded_S));
		++(*allocated[i]);
	}
	clobber();

	for (unsigned long long i = 0; i < (expanded_T - expanded_A)/expanded_S; i++)
	{
		unsigned long long idx = i % expanded_A;
		alloc.deallocate(allocated[idx], expanded_S);
		allocated.emplace(allocated.begin() + idx, alloc.allocate(expanded_S));
		++(*allocated[idx]);
		if (idx == (expanded_T - expanded_A) / expanded_S - 1) {
			clobber();
		}
	}

	std::clock_t c_end = std::clock();
	std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " " << std::flush;

	for (unsigned long long i = 0; i < expanded_A; i++) {
		alloc.deallocate(allocated[i], expanded_S);
	}
}

void run_row(size_t T, size_t A, size_t S) {
	// T = Total amount of memory to be allocated (power of 2)
	// A = Active memory - maximum amount of memory to be allocated at a time (power of 2)
	// S = Size in bytes of chunk to be allocated (power of 2)
	unsigned long long expanded_T = 1ull << T;
	unsigned long long expanded_A = 1ull << A;
	unsigned long long expanded_S = 1ull << S;

	unsigned long long chunks = 1ull << (A - S);

#ifdef DEBUG_V1
	std::cout << "T: 2^" << T << "=" << expanded_T << " A: 2^" << A << "=" << expanded_A << " S: 2^" << S << "=" << expanded_S << " Chunks: " << chunks << std::endl;
#endif // DEBUG_V1

	std::cout << "T=2^" << T << " A=2^" << A << " S=2^" << S << std::flush;

	for (size_t i = 0; i < 8; i++)
	{
		int pid = fork();
		if (pid == 0) { // Child process
			switch (i)
			{
			case 0: {
				std::allocator<char> alloc;
				run_allocation<std::allocator<char>>(expanded_T, expanded_A, expanded_S, alloc);
				break;
			}
			case 1: {
				BloombergLP::bslma::NewDeleteAllocator alloc;
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 2: {
				BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
				run_allocation<typename alloc_adaptors<char>::monotonic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 3: {
				BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 4: {
				BloombergLP::bdlma::MultipoolAllocator alloc;
				run_allocation<typename alloc_adaptors<char>::multipool>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 5: {
				BloombergLP::bdlma::MultipoolAllocator alloc;
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 6: {
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				run_allocation<typename alloc_adaptors<char>::multipool>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 7: {
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			default:
				break;
			}
			exit(0);
		}
		else {
			int error;
			wait(&error);
			if (error) {
				std::cout << "Error code " << error << "at T: " << T << " A: " << A << " S: " << S << std::endl;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	//TODO:
	// 1) Providing pre-allocated pool to monotonic - best size? Does it even make sense?
	std::cout << "Started" << std::endl;
	
	run_row(30, 15, 10);

	std::cout << "Done" << std::endl;
}
