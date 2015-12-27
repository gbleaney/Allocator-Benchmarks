
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
#include <assert.h>

using namespace BloombergLP;

// Global Variables
#define DEBUG
//#define DEBUG_V4

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

size_t hash_value = 0;
template <typename T>
struct hash {
	typedef std::size_t result_type;
	typedef T argument_type;
	result_type operator()(T obj) { return hash_value++; }
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
	typedef alloc_containers::DS1<allocators<int>::monotonic> DS1_mono;
	typedef alloc_containers::DS1<allocators<int>::multipool> DS1_multi;
	typedef alloc_containers::DS1<allocators<int>::polymorphic> DS1_poly;

	typedef alloc_containers::DS2<string::monotonic, allocators<string::monotonic>::monotonic> DS2_mono;
	typedef alloc_containers::DS2<string::multipool, allocators<string::multipool>::multipool> DS2_multi;
	typedef alloc_containers::DS2<string::polymorphic, allocators<string::polymorphic>::polymorphic> DS2_poly;

	typedef alloc_containers::DS3<allocators<int>::monotonic> DS3_mono;
	typedef alloc_containers::DS3<allocators<int>::multipool> DS3_multi;
	typedef alloc_containers::DS3<allocators<int>::polymorphic> DS3_poly;
	
	typedef alloc_containers::DS4<string::monotonic, allocators<string::monotonic>::monotonic> DS4_mono;
	typedef alloc_containers::DS4<string::multipool, allocators<string::multipool>::multipool> DS4_multi;
	typedef alloc_containers::DS4<string::polymorphic, allocators<string::polymorphic>::polymorphic> DS4_poly;

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

template<typename DS5>
struct process_DS5 {
	void operator() (DS5 *ds5, size_t elements) {
		escape(ds5);
		for (size_t i = 0; i < elements; i++) {
			ds5->emplace_back();
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
			ds6->emplace_back();
			ds6->back().reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				ds6->back().emplace_back(&random_data[random_positions[j]], random_lengths[j]);
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
			ds7->emplace_back();
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
			ds8->emplace_back();
			ds8->back().reserve(1 << 7);
			for (size_t j = 0; j < (1 << 7); j++)
			{
				ds8->back().emplace(&random_data[random_positions[j]], random_lengths[j]);
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
				inner.emplace_back(&random_data[random_positions[j]], random_lengths[j]);
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
				inner.emplace(&random_data[random_positions[j]], random_lengths[j]);
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

void run_base_loop(void (*func)(unsigned long long, size_t), std::string header) {
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

void run_nested_loop(void(*func)(unsigned long long, size_t), std::string header) {
	std::cout << header << std::endl;
#ifdef DEBUG
	short max_element_exponent = 10;
	short max_element_iteration_product_exponent = 20 - 7;
#else
	short max_element_exponent = 16;
	short max_element_iteration_product_exponent = 27 - 7;
#endif // DEBUG


	for (size_t elements = 1 << 6; elements <= 1 << max_element_exponent; elements <<= 1) {
		unsigned long long iterations = (1 << max_element_iteration_product_exponent) / elements;
		func(iterations, elements);
	}
}


int main(int argc, char *argv[]) {
	std::cout << "Started" << std::endl;

	std::cout << std::endl << "Generating random numbers" << std::endl;
	fill_random();

	//std::cout << std::endl << "Creating Allocator" << std::endl << std::endl;

	//BloombergLP::bdlma::BufferedSequentialAllocator alloc(pool, sizeof(pool));
	//std::cout << std::endl << "Creating outer container" << std::endl << std::endl;

	//typename alloc_containers::DS5<allocators<combined_containers::DS1_mono>::monotonic, allocators<int>::monotonic> container(&alloc);

	//std::cout << std::endl << "Creating inner continer" << std::endl;

	//typename combined_containers::DS1_mono inner(container.get_allocator());

	//std::cout << std::endl << "Emplacing inner container" << std::endl;

	//container.emplace_back();

	//std::cout << std::endl << "Emplacing int" << std::endl;
	//container[0].emplace_back(1);

	//run_base_allocations<typename containers::DS2,
	//	typename alloc_containers<nested_allocators<base_types::DS2, base_internal_types::DS2>::monotonic>::DS2,
	//	typename alloc_containers<nested_allocators<base_types::DS2, base_internal_types::DS2>::multipool>::DS2,
	//	typename alloc_containers<nested_allocators<base_types::DS2, base_internal_types::DS2>::polymorphic>::DS2,
	//	process_DS2>(2, 2);


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
		typename alloc_containers::DS5<allocators<combined_containers::DS1_mono>::monotonic, allocators<int>::monotonic>,
		typename alloc_containers::DS5<allocators<combined_containers::DS1_multi>::multipool, allocators<int>::multipool>,
		typename alloc_containers::DS5<allocators<combined_containers::DS1_poly>::polymorphic, allocators<int>::polymorphic>,
		process_DS5>, "**DS5**");
	run_nested_loop(&run_base_allocations<typename containers::DS6,
		typename alloc_containers::DS6<string::monotonic, allocators<combined_containers::DS2_mono>::monotonic, allocators<string::monotonic>::monotonic>,
		typename alloc_containers::DS6<string::multipool, allocators<combined_containers::DS2_multi>::multipool, allocators<string::multipool>::multipool>,
		typename alloc_containers::DS6<string::polymorphic, allocators<combined_containers::DS2_poly>::polymorphic, allocators<string::polymorphic>::polymorphic>,
		process_DS6>, "**DS6**");
	run_nested_loop(&run_base_allocations<typename containers::DS7,
		typename alloc_containers::DS7<allocators<combined_containers::DS3_mono>::monotonic, allocators<int>::monotonic>,
		typename alloc_containers::DS7<allocators<combined_containers::DS3_multi>::multipool, allocators<int>::multipool>,
		typename alloc_containers::DS7<allocators<combined_containers::DS3_poly>::polymorphic, allocators<int>::polymorphic>,
		process_DS7>, "**DS7**");
	run_nested_loop(&run_base_allocations<typename containers::DS8,
		typename alloc_containers::DS8<string::monotonic, allocators<combined_containers::DS4_mono>::monotonic, allocators<string::monotonic>::monotonic>,
		typename alloc_containers::DS8<string::multipool, allocators<combined_containers::DS4_multi>::multipool, allocators<string::multipool>::multipool>,
		typename alloc_containers::DS8<string::polymorphic, allocators<combined_containers::DS4_poly>::polymorphic, allocators<string::polymorphic>::polymorphic>,
		process_DS8>, "**DS8**");
	run_nested_loop(&run_base_allocations<typename containers::DS9,
		typename alloc_containers::DS9<allocators<combined_containers::DS1_mono>::monotonic, allocators<int>::monotonic>,
		typename alloc_containers::DS9<allocators<combined_containers::DS1_multi>::multipool, allocators<int>::multipool>,
		typename alloc_containers::DS9<allocators<combined_containers::DS1_poly>::polymorphic, allocators<int>::polymorphic>,
		process_DS9>, "**DS9**");
	run_nested_loop(&run_base_allocations<typename containers::DS10,
		typename alloc_containers::DS10<string::monotonic, allocators<combined_containers::DS2_mono>::monotonic, allocators<string::monotonic>::monotonic>,
		typename alloc_containers::DS10<string::multipool, allocators<combined_containers::DS2_multi>::multipool, allocators<string::multipool>::multipool>,
		typename alloc_containers::DS10<string::polymorphic, allocators<combined_containers::DS2_poly>::polymorphic, allocators<string::polymorphic>::polymorphic>,
		process_DS10>, "**DS10**");
	run_nested_loop(&run_base_allocations<typename containers::DS11,
		typename alloc_containers::DS11<allocators<combined_containers::DS3_mono>::monotonic, allocators<int>::monotonic>,
		typename alloc_containers::DS11<allocators<combined_containers::DS3_multi>::multipool, allocators<int>::multipool>,
		typename alloc_containers::DS11<allocators<combined_containers::DS3_poly>::polymorphic, allocators<int>::polymorphic>,
		process_DS11>, "**DS11**");
	run_nested_loop(&run_base_allocations<typename containers::DS12,
		typename alloc_containers::DS12<string::monotonic, allocators<combined_containers::DS4_mono>::monotonic, allocators<string::monotonic>::monotonic>,
		typename alloc_containers::DS12<string::multipool, allocators<combined_containers::DS4_multi>::multipool, allocators<string::multipool>::multipool>,
		typename alloc_containers::DS12<string::polymorphic, allocators<combined_containers::DS4_poly>::polymorphic, allocators<string::polymorphic>::polymorphic>,
		process_DS12>, "**DS12**");

	std::cout << "Done" << std::endl;
}
