
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

// Debugging
#include <typeinfo>
#include <assert.h>

using namespace BloombergLP;

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
// Source: https://www.youtube.com/watch?v=nXaxk27zwlk
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
