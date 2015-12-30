
#include <iostream>
#include <iomanip>
#include <memory>
#include <random>
#include <iterator>
#include <functional>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>


#include <bsl_memory.h>
#include <bslma_testallocator.h>
#include <bslma_newdeleteallocator.h>
#include <bsls_stopwatch.h>

#include <bdlma_sequentialpool.h>
#include <bdlma_sequentialallocator.h>
#include <bdlma_bufferedsequentialallocator.h>
#include <bdlma_multipoolallocator.h>

#include <vector>
#include <list>
#include <scoped_allocator>

#include "benchmark_common.h"

// Debugging
#include <typeinfo>
#include <assert.h>

using namespace BloombergLP;

// Global Variables
#define DEBUG
#define DEBUG_V1
//#define DEBUG_V2
//#define DEBUG_V3
//#define DEBUG_V4

#ifdef DEBUG
int AF_RF_PRODUCT = 256;
#else
int AF_RF_PRODUCT = 2560;
#endif // DEBUG


// Convenience typedefs
struct lists {
	typedef std::list<int> def;
	typedef std::list<int, typename allocators<int>::newdel> newdel;
	typedef std::list<int, typename allocators<int>::monotonic> monotonic;
	typedef std::list<int, typename allocators<int>::multipool> multipool;
	typedef std::list<int, bsl::allocator<int>> polymorphic;
};

struct vectors {
	typedef std::vector<lists::def> def;
	typedef std::vector<lists::newdel, typename allocators<lists::newdel>::newdel> newdel;
	typedef std::vector<lists::monotonic, typename allocators<lists::monotonic>::monotonic> monotonic;
	typedef std::vector<lists::multipool, typename allocators<lists::multipool>::multipool> multipool;
	typedef std::vector<lists::polymorphic, bsl::allocator<lists::polymorphic>> polymorphic;
};

template<typename VECTOR>
double access_lists(VECTOR *vec, int af, int rf) {
#ifdef DEBUG_V3
	std::cout << "Accessing Lists" << std::endl;
#endif // DEBUG_V3

	std::clock_t c_start = std::clock();

	for (size_t r = 0; r < rf; r++)	{
		for (size_t i = 0; i < vec->size(); i++) {
			for (size_t a = 0; a < af; a++) {
				for (auto it = (*vec)[i].begin(); it != (*vec)[i].end(); ++it) {
					(*it)++; // Increment int to cause loop to have some effect
				}
				clobber(); // TODO will this hurt caching?
			}
		}
	}
	std::clock_t c_end = std::clock();
	return (c_end - c_start) * 1.0 / CLOCKS_PER_SEC;
}

double run_combination(int G = -20, int S = 18, int af = 32, int sf = -3, int rf = 8) {
	// G  = Total system size (# subsystems * elements in subsystems). Given as power of 2 (size really = 2^G)
	// S  = Elements per subsystem. Given as power of 2 (size really = 2^S)
	// af = Access Factor - Number of iterations through a subsystem (linked list) before moving to the next
	// sf = Shuffle Factor - Number of elements popped from each list and pushed to a randomly chosen list
	//						 Note: -ve value means access occurs before shuffle
	// rf = Repeat Factor - Number of times the subsystems are iterated over


	int k = std::abs(G) - std::abs(S);
	size_t expanded_S = 1, expanded_k = 1;
	expanded_S <<= S;
	expanded_k <<= k;

#ifdef DEBUG_V3
	std::cout << "Total number of lists (k) = 2^" << k << " (aka " << expanded_k << ")" << std::endl;
	std::cout << "Total number of elements per sub system (S) = 2^" << S << " (aka " << expanded_S << ")" << std::endl;
#endif // DEBUG_V3


	// Create data under test
	typename vectors::def vec;
	vec.reserve(expanded_k);
	for (size_t i = 0; i < expanded_k; i++)
	{
		vec.emplace_back();
		for (size_t j = 0; j < expanded_S; j++)
		{
			vec.back().emplace_back((int)j);
		}
	}

	double result = 0.0;
	if (sf < 0) {
		// Access the data
		result = access_lists(&vec, af, rf);
	}

	// Shuffle the data
#ifdef DEBUG_V3
	std::cout << "Shuffling data " << std::abs(sf) << " times" << std::endl;
#endif // DEBUG_V3

	std::default_random_engine generator(1); // Consistent seed to get the same (pseudo) random distribution each time
	std::uniform_int_distribution<size_t> position_distribution(0, vec.size() - 1);
	for (size_t i = 0; i < std::abs(sf); i++) {
		for (size_t j = 0; j < vec.size(); j++)	{
			vec[position_distribution(generator)].emplace_back(vec[j].front());
			vec[j].pop_front();
		}
	}

	if (sf > 0) {
		// Access the data
		result = access_lists(&vec, af, rf);
	}
	return result;
}


int main(int argc, char *argv[]) {
	// TODO: Notes:
	// 1) Incremented int by 1 on each iteration of af, to prevent compiler optimizing away loop (also used Chandler's
	//    optimizer-defeating functions to access the ints after each iteration -- could this be a problem with caching?)

	// For baseline, G = 10^7, af = 10

	std::cout << "Started" << std::endl;

	AF_RF_PRODUCT;

	std::cout << "Problem Size 2^21 Without Allocators" << std::endl;

	int G = 21;
	int sf = 5;
	for (int S = 21; S >= 0; S--) {
		for (int af = 256; af >= 1; af >>= 1) {
			int rf = AF_RF_PRODUCT / af;
#ifdef DEBUG_V3
			std::cout << "G: " << G << " S: " << S << " af: " << af << " sf: " << sf << " rf: " << rf << std::endl;
#endif
			int pid = fork();
			if (pid == 0) { // Child process
				double result = run_combination(G, S, af, sf, rf);
				std::cout << result << " ";
				exit(0);
			}
			else {
				wait(NULL);
			}
		}
		std::cout << std::endl;
	}

	std::cout << "Done" << std::endl;
}
