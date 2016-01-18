#define DEBUG
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

#include <vector>
#include <string>
#include <unordered_set>

#include "benchmark_common.h"

// Debugging
#include <typeinfo>
#include <assert.h>

using namespace BloombergLP;

// Global Variables
const size_t RANDOM_SIZE = 1000000;
const size_t RANDOM_DATA_POINTS = 1 << 16;
const size_t RANDOM_LENGTH_MIN = 33;
const size_t RANDOM_LENGTH_MAX = 1000;


alignas(long long) static char pool[1ull << 30];
char random_data[RANDOM_SIZE];
size_t random_positions[RANDOM_DATA_POINTS];
size_t random_lengths[RANDOM_DATA_POINTS];

// Setup Functions
void fill_random() {
	std::default_random_engine generator(1); // Consistent seed to get the same (pseudo) random distribution each time
	std::uniform_int_distribution<char> char_distribution(CHAR_MIN, CHAR_MAX);
	std::uniform_int_distribution<size_t> position_distribution(0, RANDOM_SIZE - RANDOM_LENGTH_MAX);
	std::uniform_int_distribution<size_t> length_distribution(RANDOM_LENGTH_MIN, RANDOM_LENGTH_MAX);


	for (size_t i = 0; i < RANDOM_SIZE; i++)
	{
		random_data[i] = char_distribution(generator);
	}
	for (size_t i = 0; i < RANDOM_DATA_POINTS; i++)
	{
		random_positions[i] = position_distribution(generator);
		random_lengths[i] = length_distribution(generator);
	}
}


// TODO will this hashing algorithm cause issues? Maybe just a singleton counter?

size_t hash_value = 0;
template <typename T>
struct hash {
	typedef std::size_t result_type;
	typedef T argument_type;
	result_type operator()(T obj) const { return hash_value++; }
};

template <typename T>
struct equal {
	bool operator()(T const& t, T const& u) const
	{
#ifdef DEBUG_V4
		std::cout << "Comparing " << typeid(T).name() << std::endl;
#endif
		return &t == &u;
	}
};

// Convenience Typedefs

struct string {
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::monotonic> monotonic;
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::multipool> multipool;
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::newdel> newdel;
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::polymorphic> polymorphic;
};

struct containers {
	typedef std::vector<int> DS1;
	typedef std::vector<std::string> DS2;
	typedef std::unordered_set<int, hash<int>, equal<int>> DS3;
	typedef std::unordered_set<std::string, hash<std::string>, equal<std::string>> DS4;
	typedef std::vector<DS1> DS5;
	typedef std::vector<DS2> DS6;
	typedef std::vector<DS3> DS7;
	typedef std::vector<DS4> DS8;
	typedef std::unordered_set<DS1, hash<DS1>, equal<DS1>> DS9;
	typedef std::unordered_set<DS2, hash<DS2>, equal<DS2>> DS10;
	typedef std::unordered_set<DS3, hash<DS3>, equal<DS3>> DS11;
	typedef std::unordered_set<DS4, hash<DS4>, equal<DS4>> DS12;
};


struct alloc_containers {
	template<typename ALLOC>
	using DS1 = std::vector<int, ALLOC>;
	template<typename STRING, typename ALLOC>
	using DS2 = std::vector<STRING, ALLOC>;
	template<typename ALLOC>
	using DS3 = std::unordered_set<int, hash<int>, equal<int>, ALLOC>;
	template<typename STRING, typename ALLOC>
	using DS4 = std::unordered_set<STRING, hash<STRING>, equal<STRING>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS5 = std::vector<DS1<INNER_ALLOC>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS6 = std::vector<DS2<STRING, INNER_ALLOC>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS7 = std::vector<DS3<INNER_ALLOC>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS8 = std::vector<DS4<STRING, INNER_ALLOC>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS9 = std::unordered_set<DS1<INNER_ALLOC>, hash<DS1<INNER_ALLOC>>, equal<DS1<INNER_ALLOC>>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS10 = std::unordered_set<DS2<STRING, INNER_ALLOC>, hash<DS2<STRING, INNER_ALLOC>>, equal<DS2<STRING, INNER_ALLOC>>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS11 = std::unordered_set<DS3<INNER_ALLOC>, hash<DS3<INNER_ALLOC>>, equal<DS3<INNER_ALLOC>>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS12 = std::unordered_set<DS4<STRING, INNER_ALLOC>, hash<DS4<STRING, INNER_ALLOC>>, equal<DS4<STRING, INNER_ALLOC>>, ALLOC>;
};

struct combined_containers {
	typedef alloc_containers::DS1<alloc_adaptors<int>::monotonic> DS1_mono;
	typedef alloc_containers::DS1<alloc_adaptors<int>::multipool> DS1_multi;
	typedef alloc_containers::DS1<alloc_adaptors<int>::polymorphic> DS1_poly;

	typedef alloc_containers::DS2<string::monotonic, alloc_adaptors<string::monotonic>::monotonic> DS2_mono;
	typedef alloc_containers::DS2<string::multipool, alloc_adaptors<string::multipool>::multipool> DS2_multi;
	typedef alloc_containers::DS2<string::polymorphic, alloc_adaptors<string::polymorphic>::polymorphic> DS2_poly;

	typedef alloc_containers::DS3<alloc_adaptors<int>::monotonic> DS3_mono;
	typedef alloc_containers::DS3<alloc_adaptors<int>::multipool> DS3_multi;
	typedef alloc_containers::DS3<alloc_adaptors<int>::polymorphic> DS3_poly;
	
	typedef alloc_containers::DS4<string::monotonic, alloc_adaptors<string::monotonic>::monotonic> DS4_mono;
	typedef alloc_containers::DS4<string::multipool, alloc_adaptors<string::multipool>::multipool> DS4_multi;
	typedef alloc_containers::DS4<string::polymorphic, alloc_adaptors<string::polymorphic>::polymorphic> DS4_poly;

};


// Functors to exercise the data structures
template<typename DS1>
struct process_DS1 {
	void operator() (DS1 *ds1, size_t elements) {
		escape(ds1);
		for (size_t i = 0; i < elements; i++) {
			ds1->emplace_back((int)i);
		}
		clobber();
	}
};

template<typename DS2>
struct process_DS2 {
	void operator() (DS2 *ds2, size_t elements) {
		escape(ds2);
		for (size_t i = 0; i < elements; i++) {
			ds2->emplace_back(&random_data[random_positions[i]], random_lengths[i], ds2->get_allocator());
		}
		clobber();
	}
};

template<typename DS3>
struct process_DS3 {
	void operator() (DS3 *ds3, size_t elements) {
		escape(ds3);
		for (size_t i = 0; i < elements; i++) {
			ds3->emplace((int)i);
		}
		clobber();
	}
};

template<typename DS4>
struct process_DS4 {
	void operator() (DS4 *ds4, size_t elements) {
		escape(ds4);
		for (size_t i = 0; i < elements; i++) {
			ds4->emplace(&random_data[random_positions[i]], random_lengths[i], ds4->get_allocator());
		}
		clobber();
	}
};

template<typename DS5>
struct process_DS5 {
	void operator() (DS5 *ds5, size_t elements) {
		escape(ds5);
		for (size_t i = 0; i < elements; i++) {
			ds5->emplace_back(ds5->get_allocator());
			ds5->back().reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				ds5->back().emplace_back((int)j);
			}
			
		}
		clobber();
	}
};

template<typename DS6>
struct process_DS6 {
	void operator() (DS6 *ds6, size_t elements) {
		escape(ds6);
		for (size_t i = 0; i < elements; i++) {
			ds6->emplace_back(ds6->get_allocator());
			ds6->back().reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				ds6->back().emplace_back(&random_data[random_positions[j]], random_lengths[j], ds6->get_allocator());
			}

		}
		clobber();
	}
};

template<typename DS7>
struct process_DS7 {
	void operator() (DS7 *ds7, size_t elements) {
		escape(ds7);
		for (size_t i = 0; i < elements; i++) {
			ds7->emplace_back(ds7->get_allocator());
			ds7->back().reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				ds7->back().emplace((int)j);
			}

		}
		clobber();
	}
};

template<typename DS8>
struct process_DS8 {
	void operator() (DS8 *ds8, size_t elements) {
		escape(ds8);
		for (size_t i = 0; i < elements; i++) {
			ds8->emplace_back(ds8->get_allocator());
			ds8->back().reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				ds8->back().emplace(&random_data[random_positions[j]], random_lengths[j], ds8->get_allocator());
			}

		}
		clobber();
	}
};

template<typename DS9>
struct process_DS9 {
	void operator() (DS9 *ds9, size_t elements) {
		escape(ds9);
		for (size_t i = 0; i < elements; i++) {
			typename DS9::value_type inner(ds9->get_allocator());
			inner.reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				inner.emplace_back((int)j);
			}

			auto pair = ds9->emplace(std::move(inner)); // Pair of iterator to element and success
			assert(ds9->size() == i + 1); // TODO remove
		}
		clobber();
	}
};

template<typename DS10>
struct process_DS10 {
	void operator() (DS10 *ds10, size_t elements) {
		escape(ds10);
		for (size_t i = 0; i < elements; i++) {
			typename DS10::value_type inner(ds10->get_allocator());
			inner.reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				inner.emplace_back(&random_data[random_positions[j]], random_lengths[j], ds10->get_allocator());
			}

			auto pair = ds10->emplace(std::move(inner)); // Pair of iterator to element and success
			assert(ds10->size() == i + 1); // TODO remove
		}
		clobber();
	}
};

template<typename DS11>
struct process_DS11 {
	void operator() (DS11 *ds11, size_t elements) {
		escape(ds11);
		for (size_t i = 0; i < elements; i++) {
			typename DS11::value_type inner(ds11->get_allocator());
			inner.reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				inner.emplace((int)j);
			}

			auto pair = ds11->emplace(std::move(inner)); // Pair of iterator to element and success
			assert(ds11->size() == i + 1); // TODO remove
		}
		clobber();
	}
};

template<typename DS12>
struct process_DS12 {
	void operator() (DS12 *ds12, size_t elements) {
		escape(ds12);
		for (size_t i = 0; i < elements; i++) {
			typename DS12::value_type inner(ds12->get_allocator());
			inner.reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				inner.emplace(&random_data[random_positions[j]], random_lengths[j], ds12->get_allocator());
			}

			auto pair = ds12->emplace(std::move(inner)); // Pair of iterator to element and success
			assert(ds12->size() == i + 1); // TODO remove
		}
		clobber();
	}
};


template<typename GLOBAL_CONT, typename MONO_CONT, typename MULTI_CONT, typename POLY_CONT, template<typename CONT> class PROCESSER>
static void run_base_allocations(unsigned long long iterations, size_t elements) {
	// TODO: 
	// 1) Assert allocation size << sizeof(pool)
	// 2) Does dereferencing container in "wink" sections in order to pass to processor have a negative effect?
	// 3) Does switching from buffered sequential allocator to pool make any differance?
	// 4) Is it really fair that, monotonic has one big chunk handed to it, but multipool grows organically?
	// 5) What is the point of backing a monotonic with a multipool if we already supply it with enough initial memory?
	// 6) For DS9-12, inner containers must be constructed and then passed in (thus incuring the copy/move cost) because contents of a set can't be modified

	std::clock_t c_start;
	std::clock_t c_end;

#ifdef DEBUG_V1
	std::cout << std::endl << "AS1" << std::endl;
#endif

	// AS1 - Global Default
	{
		int pid = fork();
		if (pid == 0) {
				PROCESSER<GLOBAL_CONT> processer;
				c_start = std::clock();
				for (unsigned long long i = 0; i < iterations; i++) {
					GLOBAL_CONT container;
					container.reserve(elements);
					processer(&container, elements);
				}
				c_end = std::clock();
				std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
				exit(0);
		} else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS2" << std::endl;
#endif
	// AS2 - Global Default with Virtual
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bslma::NewDeleteAllocator alloc;
				POLY_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}


#ifdef DEBUG_V1
	std::cout << std::endl << "AS3" << std::endl;
#endif
	// AS3 - Monotonic
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<MONO_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
				MONO_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}


#ifdef DEBUG_V1
	std::cout << std::endl << "AS4" << std::endl;
#endif
	// AS4 - Monotonic with wink
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<MONO_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
				MONO_CONT* container = new(alloc) MONO_CONT(&alloc);
				container->reserve(elements);
				processer(container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS5" << std::endl;
#endif

	// AS5 - Monotonic with Virtual
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator  alloc(pool, sizeof(pool));
				POLY_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS6" << std::endl;
#endif

	// AS6 - Monotonic with Virtual and Wink
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator  alloc(pool, sizeof(pool));
				POLY_CONT* container = new(alloc) POLY_CONT(&alloc);
				container->reserve(elements);
				processer(container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS7" << std::endl;
#endif

	// AS7 - Multipool
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<MULTI_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::MultipoolAllocator alloc;
				MULTI_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS8" << std::endl;
#endif

	// AS8 - Multipool with wink
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<MULTI_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::MultipoolAllocator alloc;
				MULTI_CONT* container = new(alloc) MULTI_CONT(&alloc);
				container->reserve(elements);
				processer(container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS9" << std::endl;
#endif

	// AS9 - Multipool with Virtual
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::MultipoolAllocator alloc;
				POLY_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS10" << std::endl;
#endif

	// AS10 - Multipool with Virtual and Wink
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::MultipoolAllocator alloc;
				POLY_CONT* container = new(alloc) POLY_CONT(&alloc);
				container->reserve(elements);
				processer(container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS11" << std::endl;
#endif

	// AS11 - Multipool backed by Monotonic
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<MULTI_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				MULTI_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS12" << std::endl;
#endif

	// AS12 - Multipool backed by Monotonic with wink
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<MULTI_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				MULTI_CONT* container = new(alloc) MULTI_CONT(&alloc);
				container->reserve(elements);
				processer(container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS13" << std::endl;
#endif

	// AS13 - Multipool backed by Monotonic with Virtual
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				POLY_CONT container(&alloc);
				container.reserve(elements);
				processer(&container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS14" << std::endl;
#endif

	// AS14 - Multipool backed by Monotonic with Virtual and Wink
	{
		int pid = fork();
		if (pid == 0) {
			PROCESSER<POLY_CONT> processer;
			c_start = std::clock();
			for (unsigned long long i = 0; i < iterations; i++) {
				BloombergLP::bdlma::BufferedSequentialAllocator underlying_alloc(pool, sizeof(pool));
				BloombergLP::bdlma::MultipoolAllocator  alloc(&underlying_alloc);
				POLY_CONT* container = new(alloc) POLY_CONT(&alloc);
				container->reserve(elements);
				processer(container, elements);
			}
			c_end = std::clock();
			std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
			exit(0);
		}
		else {
			wait(NULL);
		}
	}

	std::cout << std::endl;
}

void run_base_loop(void(*func)(unsigned long long, size_t), std::string header) {
	std::cout << header << std::endl;
#ifdef DEBUG
	short max_element_exponent = 16;
	short max_element_iteration_product_exponent = 23;
#else
	short max_element_exponent = 16;
	short max_element_iteration_product_exponent = 27;
#endif // DEBUG


	for (unsigned long long elements = 1ull << 6; elements <= 1ull << max_element_exponent; elements <<= 1) {
		unsigned long long iterations = (1ull << max_element_iteration_product_exponent) / elements;
		std::cout << "Itr=" << iterations << " Elems=" << elements << " " << std::flush;
		func(iterations, elements);
	}
}

void run_nested_loop(void(*func)(unsigned long long, size_t), std::string header) {
	std::cout << header << std::endl;
#ifdef DEBUG
	short max_element_exponent = 16;
	short max_element_iteration_product_exponent = 23 - 7;
#else
	short max_element_exponent = 16;
	short max_element_iteration_product_exponent = 27 - 7;
#endif // DEBUG


	for (unsigned long long elements = 1ull << 6; elements <= 1ull << max_element_exponent; elements <<= 1) { // TODO: 6
		unsigned long long iterations = (1ull << max_element_iteration_product_exponent) / elements;
		std::cout << "Itr=" << iterations << " Elems=" << elements << " " << std::flush;
		func(iterations, elements);
	}
}


int main(int argc, char *argv[]) {
	std::cout << "Started" << std::endl;

	std::cout << std::endl << "Generating random numbers" << std::endl;
	fill_random();

	run_base_loop(&run_base_allocations<typename containers::DS1,
		typename combined_containers::DS1_mono,
		typename combined_containers::DS1_multi,
		typename combined_containers::DS1_poly,
		process_DS1>, "**DS1**");
	run_base_loop(&run_base_allocations<typename containers::DS2,
		typename combined_containers::DS2_mono,
		typename combined_containers::DS2_multi,
		typename combined_containers::DS2_poly,
		process_DS2>, "**DS2**");
	run_base_loop(&run_base_allocations<typename containers::DS3,
		typename combined_containers::DS3_mono,
		typename combined_containers::DS3_multi,
		typename combined_containers::DS3_poly,
		process_DS3>, "**DS3**");
	run_base_loop(&run_base_allocations<typename containers::DS4,
		typename combined_containers::DS4_mono,
		typename combined_containers::DS4_multi,
		typename combined_containers::DS4_poly,
		process_DS4>, "**DS4**");
	run_nested_loop(&run_base_allocations<typename containers::DS5,
		typename alloc_containers::DS5<alloc_adaptors<combined_containers::DS1_mono>::monotonic, alloc_adaptors<int>::monotonic>,
		typename alloc_containers::DS5<alloc_adaptors<combined_containers::DS1_multi>::multipool, alloc_adaptors<int>::multipool>,
		typename alloc_containers::DS5<alloc_adaptors<combined_containers::DS1_poly>::polymorphic, alloc_adaptors<int>::polymorphic>,
		process_DS5>, "**DS5**");
	run_nested_loop(&run_base_allocations<typename containers::DS6,
		typename alloc_containers::DS6<string::monotonic, alloc_adaptors<combined_containers::DS2_mono>::monotonic, alloc_adaptors<string::monotonic>::monotonic>,
		typename alloc_containers::DS6<string::multipool, alloc_adaptors<combined_containers::DS2_multi>::multipool, alloc_adaptors<string::multipool>::multipool>,
		typename alloc_containers::DS6<string::polymorphic, alloc_adaptors<combined_containers::DS2_poly>::polymorphic, alloc_adaptors<string::polymorphic>::polymorphic>,
		process_DS6>, "**DS6**");
	run_nested_loop(&run_base_allocations<typename containers::DS7,
		typename alloc_containers::DS7<alloc_adaptors<combined_containers::DS3_mono>::monotonic, alloc_adaptors<int>::monotonic>,
		typename alloc_containers::DS7<alloc_adaptors<combined_containers::DS3_multi>::multipool, alloc_adaptors<int>::multipool>,
		typename alloc_containers::DS7<alloc_adaptors<combined_containers::DS3_poly>::polymorphic, alloc_adaptors<int>::polymorphic>,
		process_DS7>, "**DS7**");
	run_nested_loop(&run_base_allocations<typename containers::DS8,
		typename alloc_containers::DS8<string::monotonic, alloc_adaptors<combined_containers::DS4_mono>::monotonic, alloc_adaptors<string::monotonic>::monotonic>,
		typename alloc_containers::DS8<string::multipool, alloc_adaptors<combined_containers::DS4_multi>::multipool, alloc_adaptors<string::multipool>::multipool>,
		typename alloc_containers::DS8<string::polymorphic, alloc_adaptors<combined_containers::DS4_poly>::polymorphic, alloc_adaptors<string::polymorphic>::polymorphic>,
		process_DS8>, "**DS8**");
	run_nested_loop(&run_base_allocations<typename containers::DS9,
		typename alloc_containers::DS9<alloc_adaptors<combined_containers::DS1_mono>::monotonic, alloc_adaptors<int>::monotonic>,
		typename alloc_containers::DS9<alloc_adaptors<combined_containers::DS1_multi>::multipool, alloc_adaptors<int>::multipool>,
		typename alloc_containers::DS9<alloc_adaptors<combined_containers::DS1_poly>::polymorphic, alloc_adaptors<int>::polymorphic>,
		process_DS9>, "**DS9**");
	run_nested_loop(&run_base_allocations<typename containers::DS10,
		typename alloc_containers::DS10<string::monotonic, alloc_adaptors<combined_containers::DS2_mono>::monotonic, alloc_adaptors<string::monotonic>::monotonic>,
		typename alloc_containers::DS10<string::multipool, alloc_adaptors<combined_containers::DS2_multi>::multipool, alloc_adaptors<string::multipool>::multipool>,
		typename alloc_containers::DS10<string::polymorphic, alloc_adaptors<combined_containers::DS2_poly>::polymorphic, alloc_adaptors<string::polymorphic>::polymorphic>,
		process_DS10>, "**DS10**");
	run_nested_loop(&run_base_allocations<typename containers::DS11,
		typename alloc_containers::DS11<alloc_adaptors<combined_containers::DS3_mono>::monotonic, alloc_adaptors<int>::monotonic>,
		typename alloc_containers::DS11<alloc_adaptors<combined_containers::DS3_multi>::multipool, alloc_adaptors<int>::multipool>,
		typename alloc_containers::DS11<alloc_adaptors<combined_containers::DS3_poly>::polymorphic, alloc_adaptors<int>::polymorphic>,
		process_DS11>, "**DS11**");
	run_nested_loop(&run_base_allocations<typename containers::DS12,
		typename alloc_containers::DS12<string::monotonic, alloc_adaptors<combined_containers::DS4_mono>::monotonic, alloc_adaptors<string::monotonic>::monotonic>,
		typename alloc_containers::DS12<string::multipool, alloc_adaptors<combined_containers::DS4_multi>::multipool, alloc_adaptors<string::multipool>::multipool>,
		typename alloc_containers::DS12<string::polymorphic, alloc_adaptors<combined_containers::DS4_poly>::polymorphic, alloc_adaptors<string::polymorphic>::polymorphic>,
		process_DS12>, "**DS12**");

	std::cout << "Done" << std::endl;
}
