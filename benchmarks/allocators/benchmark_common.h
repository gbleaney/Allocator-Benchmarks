
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
#include <bslma_newdeleteallocator.h>
#include <bdlma_bufferedsequentialallocator.h>
#include <bdlma_multipoolallocator.h>

// Debugging
#include <typeinfo>
#include <assert.h>

using namespace BloombergLP;

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
	typedef T * pointer;
	typedef const T * const_pointer;
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

	// TODO: Validate that these do not have unintended consequences
	template<typename OTHER>
	struct rebind
	{
		typedef alloc_adaptor<OTHER, ALLOC> other;
	};

	template<typename OTHER, typename... Args>
	void construct(OTHER * object, Args &&... args)
	{
#ifdef DEBUG_V4
		std::cout << "Constructing object of type " << typeid(OTHER).name() << std::endl;
#endif
		new (object) OTHER(std::forward<Args>(args)...);
	}

	template<typename OTHER>
	void destroy(OTHER * object)
	{
#ifdef DEBUG_V4
		std::cout << "Destroying object of type " << typeid(OTHER).name() << std::endl;
#endif
		object->~OTHER();
	}
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
	typedef bsl::allocator<BASE> polymorphic;
};
