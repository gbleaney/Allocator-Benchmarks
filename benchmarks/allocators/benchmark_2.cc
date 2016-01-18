// Debugging Settings
//#define DEBUG
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
#include <sys/resource.h>
#include <math.h>

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

#ifdef DEBUG_V3
	std::cout << "Finished accessing Lists" << std::endl;
#endif // DEBUG_V3
	return (c_end - c_start) * 1.0 / CLOCKS_PER_SEC;
}


template<typename VECTOR>
void shuffle_vector(VECTOR &vec, size_t sf, size_t list_length)
{
	// Shuffle the data
#ifdef DEBUG_V3
	std::cout << "Shuffling data " << std::abs(sf) << " times" << std::endl;
#endif // DEBUG_V3

	std::default_random_engine generator(1); // Consistent seed to get the same (pseudo) random distribution each time
	std::uniform_int_distribution<size_t> position_distribution(0, vec.size() - 1);
	for (size_t i = 0; i < std::abs(sf); i++) {
		for (size_t element_idx = 0; element_idx < list_length; element_idx++) {
			for (size_t subsystem_idx = 0; subsystem_idx < vec.size(); subsystem_idx++) {

#ifdef DEBUG_V4
				std::cout << "Generating position" << std::endl;
#endif // DEBUG_V4
				size_t pos = position_distribution(generator);

				if (vec[subsystem_idx]->d_list.size() > 0) { // TODO: not quite what is in the paper
#ifdef DEBUG_V4
					std::cout << "Grabbing front element of list at " << subsystem_idx << std::endl;
#endif // DEBUG_V4
					int popped = vec[subsystem_idx]->d_list.front();

#ifdef DEBUG_V4
					std::cout << "Emplacing " << popped << " into list at " << pos << std::endl;
#endif // DEBUG_V4
					vec[pos]->d_list.emplace_back(popped);

#ifdef DEBUG_V4
					std::cout << "Popping from front of list at " << subsystem_idx << " with " << vec[subsystem_idx]->d_list.size() << " elements" << std::endl;
#endif // DEBUG_V4
					vec[subsystem_idx]->d_list.pop_front();
				}

#ifdef DEBUG_V4
				std::cout << "Finished iteration" << std::endl;
#endif // DEBUG_V4
			}
		}
	}
}

template<typename SUBSYS>
double run_combination(int G, int S, int af, int sf, int rf) {
	// G  = Total system size (# subsystems * elements per subsystem). Given as power of 2 (size really = 2^G)
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

	if (sf > 0) {
		// Shuffle data
#ifdef DEBUG_V3
		std::cout << "Shuffling data BEFORE accessing" << std::endl;
#endif // DEBUG_V3
		shuffle_vector(vec, sf, expanded_S);
	}

	result = access_lists(&vec, af, rf);

	if (sf < 0) {
		// Shuffle data - do nothing
#ifdef DEBUG_V3
		std::cout << "Shuffling data AFTER accessing - doing nothing since shuffle is not timed" << std::endl;
#endif // DEBUG_V3
	}


#ifdef DEBUG_V3
	std::cout << "Done running combination" << std::endl;
#endif // DEBUG_V3

	return result;
}

void generate_table(int G, int alloc_num, int shuffle_sign) {
	int sf = 5;
	for (int S = 21; S >= 0; S--) { // S = 21
		std::cout << "S=" << S << " " << std::flush;
		for (int af = 256; af >= 1; af >>= 1) {
			int rf = AF_RF_PRODUCT / af;
#ifdef DEBUG_V3
			std::cout << "G: " << G << " S: " << S*shuffle_sign << " af: " << af << " sf: " << sf*shuffle_sign << " rf: " << rf << std::endl;
#endif
			int pid = fork();
			if (pid == 0) { // Child process
				double result = 0;
				switch (alloc_num) {
					case 0: {
						result = run_combination<typename subsystems::def>(G, S, af, sf*shuffle_sign, rf);
						break;
					}

					case 7: {
						result = run_combination<typename subsystems::multipool>(G, S, af, sf*shuffle_sign, rf);
						break;
					}
				}
				std::cout << result << " " << std::flush;
				exit(0);
			}
			else {
				int error;
				wait(&error);
				if (error) {
					std::cout << "Error code " << error << "at G: " << G << " S: " << S << " af: " << af << " sf: " << sf*shuffle_sign << " rf: " << rf << std::endl;
				}
			}
		}
		std::cout << std::endl;
	}
}


int main(int argc, char *argv[]) {
	// TODO: Notes:
	// 1) Incremented int by 1 on each iteration of af, to prevent compiler optimizing away loop (also used Chandler's
	//    optimizer-defeating functions to access the ints after each iteration -- could this be a problem with caching?)
	// 2) Couldn't follow paper exactly, because popping off subsystems iteratively (then pushing randmonly) means that on
	//    2nd (and further) iterations through the list, some subsystems will not have an element to pop.

	// For baseline, G = 10^7, af = 10

	std::cout << "Started" << std::endl;

	std::cout << "Problem Size 2^21 Without Allocators (Table 16) -ve shuffle" << std::endl;
	generate_table(21, 0, -1);

	std::cout << "Problem Size 2^21 Without Allocators (Table 16) +ve shuffle" << std::endl;
	generate_table(21, 0, 1);

	std::cout << "Problem Size 2^21 With Allocators (Table 18) -ve shuffle" << std::endl;
	generate_table(21, 7, -1);

	std::cout << "Problem Size 2^21 With Allocators (Table 18) +ve shuffle" << std::endl;
	generate_table(21, 7, 1);

	std::cout << "Problem Size 2^25 Without Allocators (Table 17) -ve shuffle" << std::endl;
	generate_table(25, 0, -1);

	std::cout << "Problem Size 2^25 Without Allocators (Table 17) +ve shuffle" << std::endl;
	generate_table(25, 0, 1);

	std::cout << "Problem Size 2^25 With Allocators (Table 19) -ve shuffle" << std::endl;
	generate_table(25, 7, -1);

	std::cout << "Problem Size 2^25 With Allocators (Table 19) +ve shuffle" << std::endl;
	generate_table(25, 7, 1);

	std::cout << "Done" << std::endl;
}
