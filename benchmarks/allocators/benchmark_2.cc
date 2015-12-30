
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
//#define DEBUG_V4

// Convenience typedefs
struct lists {
	typedef std::list<int> default;
	typedef std::list<int, typename allocators<int>::newdel> newdel;
	typedef std::list<int, typename allocators<int>::monotonic> monotonic;
	typedef std::list<int, typename allocators<int>::multipool> multipool;
	typedef std::list<int, bsl::allocator<int>> polymorphic;
};

struct vectors {
	typedef std::vector<lists::default> default;
	typedef std::vector<lists::newdel, typename allocators<lists::newdel>::newdel> newdel;
	typedef std::vector<lists::monotonic, typename allocators<lists::monotonic>::monotonic> monotonic;
	typedef std::vector<lists::multipool, typename allocators<lists::multipool>::multipool> multipool;
	typedef std::vector<lists::polymorphic, bsl::allocator<lists::polymorphic>> polymorphic;
};

template<typename VECTOR>
void access_lists(VECTOR *vec, int af) {
	for (size_t i = 0; i < vec->size(); i++)	{
		for (size_t j = 0; j < af; j++)	{
			for (auto it = (*vec)[i].begin(); it != (*vec)[i].end(); ++it) {
				(&it)++; // Increment int to cause loop to have some effect
			}
			clobber(); // TODO will this hurt caching?
		}
	}
}

void run_combination(int G = -20, int S = 18, int af = 32, int sf = -3, int rf = 8) {
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

	std::cout << "Total number of lists (k) = 2^" << k << " (aka " << expanded_k << ")" << std::endl;

	// Create data under test
	typename vectors::default vec;
	vec.reserve(expanded_k);
	for (size_t i = 0; i < expanded_k; i++)
	{
		vec.emplace_back();
		for (size_t j = 0; j < expanded_S; j++)
		{
			vec.back().emplace_back((int)j);
		}
	}

	if (sf < 0) {
		// Access the data
		access_lists(&vec, af);
	}
	// Shuffle the data


	if (sf > 0) {
		// Access the data
		access_lists(&vec, af);
	}
}


int main(int argc, char *argv[]) {
	// TODO: Notes:
	// 1) Incremented int by 1 on each iteration of af, to prevent compiler optimizing away loop (also used Chandler's
	//    optimizer-defeating functions to access the ints after each iteration -- could this be a problem with caching?)

	// For baseline, G = 10^7, af = 10

	std::cout << "Started" << std::endl;

	std::cout << std::endl << "Generating random numbers" << std::endl;
	fill_random();

	for (size_t i = 0; i < 1; i++)
	{
		int pid = fork();
		if (pid == 0) { // Child process
			run_combination();
		}
	}

	std::cout << "Done" << std::endl;
}
