
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

#include <bsl_memory.h>
#include <bslma_testallocator.h>
#include <bslma_newdeleteallocator.h>
#include <bsls_stopwatch.h>

#include <bdlma_sequentialpool.h>
#include <bdlma_sequentialallocator.h>
#include <bdlma_bufferedsequentialallocator.h>
#include <bdlma_multipoolallocator.h>

#include <vector>
#include <string>
#include <unordered_set>
#include <scoped_allocator>

// Debugging
#include <typeinfo>

using namespace BloombergLP;

// Global Variables
#define DEBUG


const size_t RANDOM_SIZE = 1000000;
const size_t RANDOM_DATA_POINTS = 1<<16;
const size_t RANDOM_LENGTH_MIN = 33;
const size_t RANDOM_LENGTH_MAX = 1000;


alignas(long long) static char pool[1 << 30];
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

// Chandler Carruth's Optimizer-Defeating Magic
inline
void escape(void* p)
{
	asm volatile("" : : "g"(p) : "memory");
}

inline
void clobber()
{
	asm volatile("" : : : "memory");
}

// TODO will this hashing algorithm cause issues? Maybe just a singleton counter?
template <typename T>
struct hash {
	typedef std::size_t result_type;
	typedef T argument_type;
	result_type operator()(T obj) { return 0; }
};


template <typename T, typename ALLOC>
struct alloc_adaptor {
	typedef T value_type;
	typedef T& reference;
	typedef T const& const_reference;
	ALLOC* alloc;
	alloc_adaptor() : alloc(nullptr) {
#ifdef DEBUG_V4
		std::cout << "Default constructing allocator for for " << typeid(T).name() << std::endl;
#endif
	}
	alloc_adaptor(ALLOC* allo) : alloc(allo) {
#ifdef DEBUG_V4
		std::cout << "Constructing allocator for " << typeid(T).name() << " from " << typeid(ALLOC).name() << std::endl;
#endif
	}
	template <typename T2>
	alloc_adaptor(alloc_adaptor<T2, ALLOC> other) : alloc(other.alloc) {
#ifdef DEBUG_V4
		std::cout << "Constructing allocator for " << typeid(T).name() << " from  allocator " << typeid(ALLOC).name() << " for type " << typeid(T2).name() << std::endl;
#endif
	}
	T* allocate(size_t sz) {
		//char volatile* p = (char*)alloc->allocate(sz * sizeof(T));
		//*p = '\0';																					// TODO: Is this the "writing a null byte" part
		//return (T*)p;
#ifdef DEBUG_V4
		std::cout << "Allocating " << sz * sizeof(T) << " bytes for "  << sz << " " << typeid(T).name() << std::endl;
#endif
		return (T*)alloc->allocate(sz * sizeof(T));
	}
	void deallocate(void* p, size_t) {
#ifdef DEBUG_V4
		std::cout << "Deallocating" << std::endl;
#endif
		alloc->deallocate(p); }
};

template <typename T, typename A>
bool operator== (alloc_adaptor<T, A> const& a0, alloc_adaptor<T, A> const& a1) {
	return a0.alloc == a1.alloc;
}
template <typename T, typename A>
bool operator!= (alloc_adaptor<T, A> const& a0, alloc_adaptor<T, A> const& a1) {
	return !(a0 == a1);
}


// Convenience Typedefs


template< typename BASE>
struct alloc_adaptors {
	typedef alloc_adaptor<BASE, BloombergLP::bslma::NewDeleteAllocator> newdel;
	typedef alloc_adaptor<BASE, BloombergLP::bdlma::BufferedSequentialAllocator> monotonic;
	typedef alloc_adaptor<BASE, BloombergLP::bdlma::MultipoolAllocator> multipool;
};

template< typename BASE>
struct allocators {
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<BASE>::newdel> newdel;
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<BASE>::monotonic> monotonic;
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<BASE>::multipool> multipool;
	typedef std::scoped_allocator_adaptor<bsl::allocator<BASE>> polymorphic;
};

template<typename CONTAINER, typename BASE>
struct nested_allocators {
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<CONTAINER>::newdel, typename alloc_adaptors<BASE>::newdel> newdel;
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<CONTAINER>::monotonic, typename alloc_adaptors<BASE>::monotonic> monotonic;
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<CONTAINER>::multipool, typename alloc_adaptors<BASE>::multipool> multipool;
	typedef std::scoped_allocator_adaptor<bsl::allocator<CONTAINER>, bsl::allocator<BASE>> polymorphic;
};

template<typename CONTAINER, typename BASE, typename BASE_INTERNAL>
struct tripple_nested_allocators {
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<CONTAINER>::newdel, typename alloc_adaptors<BASE>::newdel, typename alloc_adaptors<BASE_INTERNAL>::newdel> newdel;
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<CONTAINER>::monotonic, typename alloc_adaptors<BASE>::monotonic, typename alloc_adaptors<BASE_INTERNAL>::monotonic> monotonic;
	typedef std::scoped_allocator_adaptor<typename alloc_adaptors<CONTAINER>::multipool, typename alloc_adaptors<BASE>::multipool, typename alloc_adaptors<BASE_INTERNAL>::multipool> multipool;
	typedef std::scoped_allocator_adaptor<bsl::allocator<CONTAINER>, bsl::allocator<BASE>, bsl::allocator<BASE_INTERNAL>> polymorphic;
};

struct string {
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::monotonic> monotonic;
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::multipool> multipool;
	typedef std::basic_string<char, std::char_traits<char>, alloc_adaptors<char>::newdel> newdel;
	typedef std::basic_string<char, std::char_traits<char>, bsl::allocator<char>> polymorphic;
};

struct containers {
	typedef std::vector<int> DS1;
	typedef std::vector<std::string> DS2;
	typedef std::unordered_set<int, hash<int>> DS3;
	typedef std::unordered_set<std::string, hash<std::string>> DS4;
	typedef std::vector<DS1> DS5;
	typedef std::vector<DS2> DS6;
	typedef std::vector<DS3> DS7;
	typedef std::vector<DS4> DS8;
	typedef std::unordered_set<DS1, hash<DS1>> DS9;
	typedef std::unordered_set<DS2, hash<DS2>> DS10;
	typedef std::unordered_set<DS3, hash<DS3>> DS11;
	typedef std::unordered_set<DS4, hash<DS4>> DS12;
};


struct alloc_containers {

	template<typename ALLOC>
	using DS1 = std::vector<int, ALLOC>;
	template<typename STRING, typename ALLOC>
	using DS2 = std::vector<STRING, ALLOC>;
	template<typename ALLOC>
	using DS3 = std::unordered_set<int, hash<int>, std::equal_to<int>, ALLOC>;
	template<typename STRING, typename ALLOC>
	using DS4 = std::unordered_set<STRING, hash<STRING>, std::equal_to<STRING>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS5 = std::vector<DS1<INNER_ALLOC>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS6 = std::vector<DS2<STRING, INNER_ALLOC>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS7 = std::vector<DS3<INNER_ALLOC>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS8 = std::vector<DS4<STRING, INNER_ALLOC>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS9 = std::unordered_set<DS1<INNER_ALLOC>, hash<DS1<INNER_ALLOC>>, std::equal_to<DS1<INNER_ALLOC>>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS10 = std::unordered_set<DS2<STRING, INNER_ALLOC>, hash<DS2<STRING, INNER_ALLOC>>, std::equal_to<DS2<STRING, INNER_ALLOC>>, ALLOC>;
	template<typename ALLOC, typename INNER_ALLOC>
	using DS11 = std::unordered_set<DS3<INNER_ALLOC>, hash<DS3<INNER_ALLOC>>, std::equal_to<DS3<INNER_ALLOC>>, ALLOC>;
	template<typename STRING, typename ALLOC, typename INNER_ALLOC>
	using DS12 = std::unordered_set<DS4<STRING, INNER_ALLOC>, hash<DS4<STRING, INNER_ALLOC>>, std::equal_to<DS4<STRING, INNER_ALLOC>>, ALLOC>;
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
			ds2->emplace_back(&random_data[random_positions[i]], random_lengths[i]);
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
			ds4->emplace(&random_data[random_positions[i]], random_lengths[i]);
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

	std::clock_t c_start;
	std::clock_t c_end;

#ifdef DEBUG_V1
	std::cout << std::endl << "AS1" << std::endl;
#endif

	// AS1 - Global Default
	{
		PROCESSER<GLOBAL_CONT> processer;
		c_start = std::clock();
		for (unsigned long long i = 0; i < iterations; i++) {
			GLOBAL_CONT container;
			container.reserve(elements);
			processer(&container, elements);
		}
		c_end = std::clock();
		std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS2" << std::endl;
#endif
	// AS2 - Global Default with Virtual
	{
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
	}


#ifdef DEBUG_V1
	std::cout << std::endl << "AS3" << std::endl;
#endif
	// AS3 - Monotonic
	{
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
	}


#ifdef DEBUG_V1
	std::cout << std::endl << "AS4" << std::endl;
#endif
	// AS4 - Monotonic with wink
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS5" << std::endl;
#endif

	// AS5 - Monotonic with Virtual
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS6" << std::endl;
#endif

	// AS6 - Monotonic with Virtual and Wink
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS7" << std::endl;
#endif

	// AS7 - Multipool
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS8" << std::endl;
#endif

	// AS8 - Multipool with wink
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS9" << std::endl;
#endif

	// AS9 - Multipool with Virtual
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS10" << std::endl;
#endif

	// AS10 - Multipool with Virtual and Wink
	{
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
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS11" << std::endl;
#endif

	// AS11 - Monotonic + Multipool
	{
		PROCESSER<MONO_CONT> processer;
		c_start = std::clock();
		for (unsigned long long i = 0; i < iterations; i++) {
			BloombergLP::bdlma::MultipoolAllocator underlying_alloc;
			BloombergLP::bdlma::BufferedSequentialAllocator  alloc(pool, sizeof(pool), &underlying_alloc);
			MONO_CONT container(&alloc);
			container.reserve(elements);
			processer(&container, elements);
		}
		c_end = std::clock();
		std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS12" << std::endl;
#endif

	// AS12 - Monotonic + Multipool with wink
	{
		PROCESSER<MONO_CONT> processer;
		c_start = std::clock();
		for (unsigned long long i = 0; i < iterations; i++) {
			BloombergLP::bdlma::MultipoolAllocator underlying_alloc;
			BloombergLP::bdlma::BufferedSequentialAllocator  alloc(pool, sizeof(pool), &underlying_alloc);
			MONO_CONT* container = new(alloc) MONO_CONT(&alloc);
			container->reserve(elements);
			processer(container, elements);
		}
		c_end = std::clock();
		std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS13" << std::endl;
#endif

	// AS13 - Monotonic + Multipool with Virtual
	{
		PROCESSER<POLY_CONT> processer;
		c_start = std::clock();
		for (unsigned long long i = 0; i < iterations; i++) {
			BloombergLP::bdlma::MultipoolAllocator underlying_alloc;
			BloombergLP::bdlma::BufferedSequentialAllocator  alloc(pool, sizeof(pool), &underlying_alloc);
			POLY_CONT container(&alloc);
			container.reserve(elements);
			processer(&container, elements);
		}
		c_end = std::clock();
		std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
	}

#ifdef DEBUG_V1
	std::cout << std::endl << "AS14" << std::endl;
#endif

	// AS14 - Monotonic + Multipool with Virtual and Wink
	{
		PROCESSER<POLY_CONT> processer;
		c_start = std::clock();
		for (unsigned long long i = 0; i < iterations; i++) {
			BloombergLP::bdlma::MultipoolAllocator underlying_alloc;
			BloombergLP::bdlma::BufferedSequentialAllocator  alloc(pool, sizeof(pool), &underlying_alloc);
			POLY_CONT* container = new(alloc) POLY_CONT(&alloc);
			container->reserve(elements);
			processer(container, elements);
		}
		c_end = std::clock();
		std::cout << (c_end - c_start) * 1.0 / CLOCKS_PER_SEC << " ";
	}

	std::cout << std::endl;
}

void run_loop(void (*func)(unsigned long long, size_t), std::string header) {
	std::cout << header << std::endl;
#ifdef DEBUG
	short max_element_exponent = 10;
	short max_element_iteration_product_exponent = 20;
#else
	short max_element_exponent = 16;
	short max_element_iteration_product_exponent = 27;
#endif // DEBUG


	for (size_t elements = 1 << 6; elements <= 1 << max_element_exponent; elements <<= 1) {
		unsigned long long iterations = (1 << max_element_iteration_product_exponent) / elements;
		func(iterations, elements);
	}
}


int main(int argc, char *argv[]) {
	std::cout << "Started" << std::endl;

	std::cout << "Generating random numbers" << std::endl;
	fill_random();

	//std::cout << str << std::endl;

	//typedef alloc_adaptor<char, BloombergLP::bdlma::BufferedSequentialAllocator> mono_adaptor_char;
	//typedef std::basic_string<char, std::char_traits<char>, mono_adaptor_char> mono_string;
	//typedef alloc_adaptor<mono_string, BloombergLP::bdlma::BufferedSequentialAllocator> mono_adaptor_string;
	//typedef std::scoped_allocator_adaptor<mono_adaptor_string> mono_allocator;

	//BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
	//std::vector<mono_string, mono_allocator> container(&alloc);
	//container.emplace_back("qwer");
	//std::cout << container[0] << std::endl;


	//run_base_allocations<typename containers::DS2,
	//	typename alloc_containers<nested_allocators<base_types::DS2, base_internal_types::DS2>::monotonic>::DS2,
	//	typename alloc_containers<nested_allocators<base_types::DS2, base_internal_types::DS2>::multipool>::DS2,
	//	typename alloc_containers<nested_allocators<base_types::DS2, base_internal_types::DS2>::polymorphic>::DS2,
	//	process_DS2>(2, 2);


	run_loop(&run_base_allocations<typename containers::DS1,
		typename alloc_containers::DS1<allocators<int>::monotonic>,
		typename alloc_containers::DS1<allocators<int>::multipool>,
		typename alloc_containers::DS1<allocators<int>::polymorphic>,
		process_DS1>, "**DS1**");
	run_loop(&run_base_allocations<typename containers::DS2,
		typename alloc_containers::DS2<string::monotonic, allocators<string::monotonic>::monotonic>,
		typename alloc_containers::DS2<string::multipool, allocators<string::multipool>::multipool>,
		typename alloc_containers::DS2<string::polymorphic, allocators<string::polymorphic>::polymorphic>,
		process_DS2>, "**DS2**");
	run_loop(&run_base_allocations<typename containers::DS3,
		typename alloc_containers::DS3<allocators<int>::monotonic>,
		typename alloc_containers::DS3<allocators<int>::multipool>,
		typename alloc_containers::DS3<allocators<int>::polymorphic>,
		process_DS3>, "**DS3**");
	run_loop(&run_base_allocations<typename containers::DS4,
		typename alloc_containers::DS4<string::monotonic, allocators<string::monotonic>::monotonic>,
		typename alloc_containers::DS4<string::multipool, allocators<string::multipool>::multipool>,
		typename alloc_containers::DS4<string::polymorphic, allocators<string::polymorphic>::polymorphic>,
		process_DS4>, "**DS4**");

	std::cout << "Done" << std::endl;
}
