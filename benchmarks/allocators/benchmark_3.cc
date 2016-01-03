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
void run_allocation(const unsigned long long T_bytes, const unsigned long long A_bytes, const unsigned long long S_bytes, ALLOC alloc) {
	// T_bytes = Total amount of memory to be allocated
	// A_bytes = Active memory. The maximum amount of memory to be allocated at a time
	// S_bytes = Size in bytes of chunk to be allocated
	// alloc = Allocator to be used to allocate memory

#ifdef DEBUG_V2
	std::cout << "Running Allocation" << std::endl;
#endif // DEBUG_V2

	const unsigned long long A_count = A_bytes / S_bytes;
	const unsigned long long remaining_count = (T_bytes - A_bytes) / S_bytes;

	std::vector<char *> allocated;
	allocated.reserve(A_count);

	escape(allocated.data());

	std::clock_t c_start = std::clock();
	
#ifdef DEBUG_V2
	std::cout << "Running initial allocation for " << allocated.size() << " chunks of " << S_bytes << " bytes" << std::endl;
#endif // DEBUG_V2
	for (unsigned long long i = 0; i < A_count; i++)	{
		allocated.emplace_back(alloc.allocate(S_bytes));
		++(*allocated[i]);
	}
	clobber();

#ifdef DEBUG_V2
	std::cout << "Running additional allocation for " << remaining_count << " chunks of " << S_bytes << " bytes" << std::endl;
#endif // DEBUG_V2
	for (unsigned long long i = 0; i < remaining_count; i++)
	{
		unsigned long long idx = i % A_count;
		alloc.deallocate(allocated[idx], S_bytes);
		allocated.emplace(allocated.begin() + idx, alloc.allocate(S_bytes));
		++(*allocated[idx]);
		if (idx == A_count - 1) {
#ifdef DEBUG_V2
			std::cout << "Clobbering memory after reaching index " << i << " in loop (" << idx << " in vector)" << std::endl;
			std::cout << "Clobber number: " << i / idx << std::endl;
#endif // DEBUG_V2
			clobber();
		}
	}

	std::clock_t c_end = std::clock();
	std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " " << std::flush;

#ifdef DEBUG_V2
	std::cout << "Deallocating remaining " << allocated.size() << " chunks" << std::endl;
#endif // DEBUG_V2
	for (unsigned long long i = 0; i < allocated.size(); i++) {
		alloc.deallocate(allocated[i], S_bytes);
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
#ifdef DEBUG_V2
				std::cout << std::endl << "AS1" << std::endl;
#endif // DEBUG_V2
				std::allocator<char> alloc;
				run_allocation<std::allocator<char>>(expanded_T, expanded_A, expanded_S, alloc);
				break;
			}
			case 1: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS2" << std::endl;
#endif // DEBUG_V2
				BloombergLP::bslma::NewDeleteAllocator alloc;
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 2: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS3" << std::endl;
#endif // DEBUG_V2
				BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
				run_allocation<typename alloc_adaptors<char>::monotonic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 3: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS5" << std::endl;
#endif // DEBUG_V2
				BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 4: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS7" << std::endl;
#endif // DEBUG_V2
				BloombergLP::bdlma::MultipoolAllocator alloc;
				run_allocation<typename alloc_adaptors<char>::multipool>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 5: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS9" << std::endl;
#endif // DEBUG_V2
				BloombergLP::bdlma::MultipoolAllocator alloc;
				run_allocation<typename alloc_adaptors<char>::polymorphic>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 6: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS11" << std::endl;
#endif // DEBUG_V2
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				run_allocation<typename alloc_adaptors<char>::multipool>(expanded_T, expanded_A, expanded_S, &alloc);
				break;
			}
			case 7: {
#ifdef DEBUG_V2
				std::cout << std::endl << "AS13" << std::endl;
#endif // DEBUG_V2
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
