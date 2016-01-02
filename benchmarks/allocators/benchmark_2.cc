// Debugging Settings
#define DEBUG
//#define DEBUG_V1
//#define DEBUG_V2
//#define DEBUG_V3
//#define DEBUG_V4

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

#include "benchmark_common.h"

// Debugging
#include <typeinfo>
#include <assert.h>

using namespace BloombergLP;

// Global Variables

#ifdef DEBUG
int AF_RF_PRODUCT = 256;
#else
int AF_RF_PRODUCT = 2560;
#endif // DEBUG

template<typename ALLOC>
class AllocSubsystem {
public:
	ALLOC d_alloc;
	std::list<int, alloc_adaptor<int, ALLOC> > d_list;
	AllocSubsystem() : d_alloc(), d_list(&d_alloc) {}
};

class DefaultSubsystem {
public:
	std::list<int> d_list;
	DefaultSubsystem() : d_list() {}
};

// Convenience typedefs
struct subsystems {
	typedef DefaultSubsystem def;
	typedef AllocSubsystem<BloombergLP::bdlma::MultipoolAllocator> multipool;
};

//struct vectors {
//	typedef std::vector<subsystems::def> def;
//	typedef std::vector<subsystems::newdel, typename alloc_adaptors<subsystems::newdel>::newdel> newdel;
//	typedef std::vector<subsystems::monotonic, typename alloc_adaptors<subsystems::monotonic>::monotonic> monotonic;
//	typedef std::vector<subsystems::multipool, typename alloc_adaptors<subsystems::multipool>::multipool> multipool;
//	typedef std::vector<subsystems::polymorphic, bsl::allocator<int>> polymorphic;
//};

template<typename VECTOR>
double access_lists(VECTOR *vec, int af, int rf) {
#ifdef DEBUG_V3
	std::cout << "Accessing Lists" << std::endl;
#endif // DEBUG_V3

	std::clock_t c_start = std::clock();

	for (size_t r = 0; r < rf; r++)	{
		for (size_t i = 0; i < vec->size(); i++) {
			for (size_t a = 0; a < af; a++) {
				for (auto it = (*vec)[i]->d_list.begin(); it != (*vec)[i]->d_list.end(); ++it) {
					(*it)++; // Increment int to cause loop to have some effect
				}
				clobber(); // TODO will this hurt caching?
			}
		}
	}
	std::clock_t c_end = std::clock();
	return (c_end - c_start) * 1.0 / CLOCKS_PER_SEC;
}

template<typename SUBSYS>
double run_combination(int G, int S, int af, int sf, int rf) {
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

	std::vector<SUBSYS *> vec;

	// Create data under test
	vec.reserve(expanded_k);
	for (size_t i = 0; i < expanded_k; i++)	{
		vec.emplace_back(new SUBSYS()); // Never deallocated because we exit immediately after this function reutrns anyway

		for (size_t j = 0; j < expanded_S; j++)
		{
			vec.back()->d_list.emplace_back((int)j);
		}
	}

#ifdef DEBUG_V3
	std::cout << "Created vector with " << vec.size() << " elements" << std::endl;
#endif // DEBUG_V3

	double result = 0.0;
	if (sf < 0) {
		// Access the data
#ifdef DEBUG_V3
		std::cout << "Accessing data BEFORE shuffling" << std::endl;
#endif // DEBUG_V3
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
			vec[position_distribution(generator)]->d_list.emplace_back(vec[j]->d_list.front());
			vec[j]->d_list.pop_front();
		}
	}

	if (sf > 0) {
		// Access the data
#ifdef DEBUG_V3
		std::cout << "Accessing data AFTER shuffling" << std::endl;
#endif // DEBUG_V3
		result = access_lists(&vec, af, rf);
	}
	return result;
}

void generate_table(int G, int alloc_num) {
	int sf = 5;
	for (int S = 21; S >= 0; S--) { // S = 21
		for (int af = 256; af >= 1; af >>= 1) {
			int rf = AF_RF_PRODUCT / af;
#ifdef DEBUG_V3
			std::cout << "G: " << G << " S: " << S << " af: " << af << " sf: " << sf << " rf: " << rf << std::endl;
#endif
			int pid = fork();
			if (pid == 0) { // Child process
				double result = 0;
				switch (alloc_num) {
					case 0: {
						result = run_combination<typename subsystems::def>(G, S, af, sf, rf);
						break;
					}

					case 7: {
						result = run_combination<typename subsystems::multipool>(G, S, af, sf, rf);
						break;
					}
				}
				std::cout << result << " ";
				exit(0);
			}
			else {
				wait(NULL);
			}
		}
		std::cout << std::endl;
	}
}


int main(int argc, char *argv[]) {
	// TODO: Notes:
	// 1) Incremented int by 1 on each iteration of af, to prevent compiler optimizing away loop (also used Chandler's
	//    optimizer-defeating functions to access the ints after each iteration -- could this be a problem with caching?)

	// For baseline, G = 10^7, af = 10

	std::cout << "Started" << std::endl;
	
	//std::vector<typename subsystems::def> vec;
	//std::allocator<int> alloc;
	//vec.emplace_back(alloc);

	//std::vector<typename subsystems::multipool> vec_1;
	//BloombergLP::bdlma::MultipoolAllocator alloc_1;
	//vec_1.emplace_back(alloc_1);

	//{
	//	std::cout << "Creating allocator" << std::endl;
	//	BloombergLP::bdlma::MultipoolAllocator alloc;
	//	std::cout << "Creating Vector" << std::endl;
	//	typename vectors::multipool vector(&alloc);
	//	std::cout << "Creating List" << std::endl;
	//	vector.emplace_back(&alloc);
	//	std::cout << "Adding to list" << std::endl;
	//	vector[0].push_back(3);
	//	std::cout << "Reading from List/Vector" << std::endl;
	//	std::cout << vector[0].back() << std::endl;
	//	std::cout << "Destroying Vector/List" << std::endl;
	//}

	std::cout << "Problem Size 2^21 Without Allocators" << std::endl;
	generate_table(21, 0);

	//std::cout << "Problem Size 2^25 Without Allocators" << std::endl;
	//generate_table(25, 0);

	std::cout << "Problem Size 2^21 With Allocators" << std::endl;
	generate_table(21, 7);

	//std::cout << "Problem Size 2^25 With Allocators" << std::endl;
	//generate_table(25, 7);

	std::cout << "Done" << std::endl;
}
