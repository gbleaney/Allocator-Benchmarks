
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
#include "allocont.h"

// Debugging
#include <typeinfo>

using namespace BloombergLP;

// Global Variables

alignas(long long) static char pool[1 << 30];


// Chandler Carruth's Optimizer-Defeating Magic
void escape(void* p)
{
	asm volatile("" : : "g"(p) : "memory");
}

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
	alloc_adaptor() : alloc(nullptr) {}
	alloc_adaptor(ALLOC* allo) : alloc(allo) {}
	template <typename T2>
	alloc_adaptor(alloc_adaptor<T2, ALLOC> other) : alloc(other.alloc) { }
	T* allocate(size_t sz) {
		//char volatile* p = (char*)alloc->allocate(sz * sizeof(T));
		//*p = '\0';																					// TODO: Is this the "writing a null byte" part
		//return (T*)p;
		return (T*)alloc->allocate(sz * sizeof(T));
	}
	void deallocate(void* p, size_t) { alloc->deallocate(p); }
};


// Convenience Typedefs

struct base_types {
	typedef int DS1;
	typedef std::string DS2;
	typedef int DS3;
	typedef std::string DS4;
	typedef int DS5;
	typedef std::string DS6;
	typedef int DS7;
	typedef std::string DS8;
	typedef int DS9;
	typedef std::string DS10;
	typedef int DS11;
	typedef std::string DS12;
};

struct base_containers {
	typedef std::vector<int> DS5;
	typedef std::vector<std::string> DS6;
	typedef std::unordered_set<int> DS7;
	typedef std::unordered_set<std::string> DS8;
	typedef std::vector<int> DS9;
	typedef std::vector<std::string> DS10;
	typedef std::unordered_set<int> DS11;
	typedef std::unordered_set<std::string> DS12;
};

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
};

struct containers {
	typedef std::vector<int> DS1;
	typedef std::vector<std::string> DS2;
	typedef std::unordered_set<int> DS3;
	typedef std::unordered_set<std::string> DS4;
	typedef std::vector<std::vector<int>> DS5;
	typedef std::vector<std::vector<std::string>> DS6;
	typedef std::vector<std::unordered_set<int>> DS7;
	typedef std::vector<std::unordered_set<std::string>> DS8;
	typedef std::unordered_set<std::vector<int>> DS9;
	typedef std::unordered_set<std::vector<std::string>> DS10;
	typedef std::unordered_set<std::unordered_set<int>> DS11;
	typedef std::unordered_set<std::unordered_set<std::string>> DS12;
};


template< typename ALLOC>
struct alloc_containers {
	typedef std::vector<int, ALLOC> DS1;
	typedef std::vector<std::string, ALLOC> DS2;
	typedef std::unordered_set<int, hash<int>, std::equal_to<int>, ALLOC> DS3;
	typedef std::unordered_set<std::string, hash<std::string>, std::equal_to<std::string>, ALLOC> DS4;
	typedef std::vector<std::vector<int>> DS5;
	typedef std::vector<std::vector<std::string>> DS6;
	typedef std::vector<std::unordered_set<int, hash<int>, std::equal_to<int>, ALLOC>> DS7;
	typedef std::vector<std::unordered_set<std::string, hash<std::string>, std::equal_to<std::string>, ALLOC>> DS8;
	typedef std::unordered_set<std::vector<int>, hash<std::vector<int>>, std::equal_to<std::vector<int>>, ALLOC> DS9;
	typedef std::unordered_set<std::vector<std::string>, hash<std::vector<std::string>>, std::equal_to<std::vector<std::string>>, ALLOC> DS10;
	typedef std::unordered_set<std::unordered_set<int>, hash<std::unordered_set<int>>, std::equal_to<std::unordered_set<int>>, ALLOC> DS11;
	typedef std::unordered_set<std::unordered_set<std::string>, hash<std::unordered_set<std::string>>, std::equal_to<std::unordered_set<std::string>>, ALLOC> DS12;
};

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

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
		std::cout << c_end - c_start << "\t";
	}

	std::cout << std::endl;
}


int main(int argc, char *argv[]) {
	std::cout << "Started" << std::endl;

	//BloombergLP::bslma::NewDeleteAllocator newalloc;
	//newalloc.allocate((size_t)1);

	//alloc_adaptor<int, BloombergLP::bslma::NewDeleteAllocator> adapted_newalloc;
	////auto ptr = adapted_newalloc.allocate((size_t)1);

	//allocators<base_types::DS1>::newdel alloc;
	////alloc.allocate((size_t)1);

	//alloc_containers<allocators<base_types::DS1>::newdel>::DS1 ds1(&newalloc);
	//ds1.reserve(1);

	//std::cout << typeid(alloc).name() << std::endl;
	//std::cout << typeid(newalloc).name() << std::endl;
	//std::cout << typeid(adapted_newalloc).name() << std::endl;


	run_base_allocations<typename containers::DS1,
						 typename alloc_containers<allocators<base_types::DS1>::monotonic>::DS1,
						 typename alloc_containers<allocators<base_types::DS1>::multipool>::DS1,
						 typename alloc_containers<allocators<base_types::DS1>::polymorphic>::DS1,
						 process_DS1>
		(5, 5);

	std::cout << "Done" << std::endl;
}







static const int max_problem_logsize = 30;

void usage(char const* cmd, int result)
{
    std::cerr <<
"usage: " << cmd << " <size> <split> [<csv>]\n"
"    size:  log2 of total element count, 20 -> 1,000,000\n"
"    split: log2 of container size, 10 -> 1,000\n"
"    csv:   if present, produce CSV: <<time>, <description>...>\n"
"    Number of containers used is 2^(size - split)\n"
"    1 <= size <= " << max_problem_logsize << ", 1 <= split <= size\n";
    exit(result);
}

// side_effect() touches memory in a way that optimizers will not optimize away

void side_effect(void* p, size_t len)
{
    static volatile thread_local char* target;
    target = (char*) p;
    memset((char*)target, 0xff, len);
}

// (not standard yet)
struct range {
    int start; int stop;
    struct iter {
        int i;
        bool operator!=(iter other) { return other.i != i; };
        iter& operator++() { ++i; return *this; };
        int operator*() { return i; }
    };
    iter begin() { return iter{start}; }
    iter end() { return iter{stop}; }
};

char trash[1 << 16];  // random characters to construct string keys from

std::default_random_engine random_engine;
std::uniform_int_distribution<int> pos_dist(0, sizeof(trash) - 1000);
std::uniform_int_distribution<int> length_dist(33, 1000);

char* sptr() { return trash + pos_dist(random_engine); }
size_t slen() {return length_dist(random_engine); }

enum {
    VEC=1<<0, HASH=1<<1,
        VECVEC=1<<2, VECHASH=1<<3,
        HASHVEC=1<<4, HASHHASH=1<<5,
    INT=1<<6, STR=1<<7,
    SA=1<<8, MT=1<<9, MTD=1<<10, PL=1<<11, PLD=1<<12,
        PM=1<<13, PMD=1<<14, CT=1<<15, RT=1<<16
};

char const* const names[] = {
    "vector", "unordered_set", "vector:vector", "vector:unordered_set",
        "unordered_set:vector", "unordered_set:unordered_set",
    "int", "string",
    "new/delete", "monotonic", "monotonic/drop", "multipool", "multipool/drop",
        "multipool/monotonic", "multipool/monotonic/drop",
    "compile-time", "run-time"
};

void print_case(int mask)
{
    for (int i = 8, m = SA; m <= RT; ++i, m <<= 1) {
        if (m & mask) {
            if (m < CT)
                std::cout << "allocator: " << names[i] << ", ";
            else
                std::cout << "bound: " << names[i];
        }}
}
void print_datastruct(int mask)
{
    for (int i = 0, m = VEC; m < SA; ++i, (m <<= 1)) {
        if (m & mask) {
            std::cout << names[i];
            if (m < INT)
                std::cout << ":";
        }}
}

template <typename Test>
double measure(int mask, bool csv, double reference, Test test)
{
    if (mask & SA)
        std::cout << std::endl;
    if (!csv) {
        if (mask & SA) {
            print_datastruct(mask);
            std::cout << ":\n\n";
        }
        std::cout << "   ";
        print_case(mask);
        std::cout << std::endl;
    }

#ifndef DEBUG
    int pipes[2];
    int result = pipe(pipes);
    if (result < 0) {
        std::cerr << "\nFailed pipe\n";
        std::cout << "\nFailed pipe\n";
        exit(-1);
    }
    union { double result_time; char buf[sizeof(double)]; };
    int pid = fork();
    if (pid < 0) {
        std::cerr << "\nFailed fork\n";
        std::cout << "\nFailed fork\n";
        exit(-1);
    } else if (pid > 0) {  // parent
        close(pipes[1]);
        int status = 0;
        waitpid(pid, &status, 0);
        if (status == 0) {
            int got = read(pipes[0], buf, sizeof(buf));
            if (got != sizeof(buf)) {
                std::cerr << "\nFailed read\n";
                std::cout << "\nFailed read\n";
                exit(-1);
            }
        } else {
            if (!csv) {
                std::cout << "   (failed)\n" << std::endl;
            } else {
                std::cout << "(failed), (failed%), ";
                print_datastruct(mask);
                std::cout << ", ";
                print_case(mask);
                std::cout << std::endl;
            }
        }
        close(pipes[0]);
    } else {  // child
        close(pipes[0]);
#else
        double result_time;
#endif
        bool failed = false;
        bsls::Stopwatch timer;
        try {
            timer.start(true);
            test();
        } catch (std::bad_alloc&) {
            failed = true;
        }
        timer.stop();

        double times[3] = { 0.0, 0.0, 0.0 };
        if (!failed)
            timer.accumulatedTimes(times, times+1, times+2);

        if (!csv) {
            if (!failed) {
                std::cout << "   sys: " << times[0]
                          << " user: " << times[1]
                          << " wall: " << times[2] << ", ";
                if ((mask & (SA|CT)) == (SA|CT)) {
                    std::cout << "(100%)\n";
                } else if (reference == 0.0) {  // reference run failed
                    std::cout << "(N/A%)\n";
                } else {
                    std::cout << (times[2] * 100.)/reference << "%\n";
                }
            } else {
                std::cout << "   (failed)\n";
            }
            std::cout << std::endl;
        } else {
            if (!failed) {
                std::cout << times[2] << ", ";
                if ((mask & (SA|CT)) == (SA|CT)) {
                    std::cout << "(100%), ";
                } else if (reference == 0.0) {  // reference run failed
                    std::cout << "(N/A%), ";
                } else {
                    std::cout << (times[2] * 100.)/reference << "%, ";
                }
            } else {
                std::cout << "(failed), (failed%), ";
            }
            print_datastruct(mask);
            std::cout << ", ";
            print_case(mask);
            std::cout << std::endl;
        }
        result_time = times[2];
#ifndef DEBUG
        write(pipes[1], buf, sizeof(buf));
        close(pipes[1]);
        exit(0);
    }
#endif

    return ((mask & (SA|CT)) == (SA|CT)) ? result_time : reference;
}

#ifdef __GLIBCXX__
namespace std {
  template<typename C, typename T, typename A>
    struct hash<basic_string<C, T, A>>
    {
      using result_type = size_t;
      using argument_type = basic_string<C, T, A>;

      result_type operator()(const argument_type& s) const noexcept
      { return std::_Hash_impl::hash(s.data(), s.length()); }
    };
}
#endif

template <typename T>
struct my_hash {
    using result_type = size_t;
    using argument_type = T;
    result_type operator()(T const& t) const
        { return 1048583 * (1 + reinterpret_cast<unsigned long>(&t)); }
};
template <typename T>
struct my_equal {
    bool operator()(T const& t, T const& u) const
        { return &t == &u; }
};

template <
    typename StdCont,
    typename MonoCont,
    typename MultiCont,
    typename PolyCont,
    typename Work>
void apply_allocation_strategies(
    int mask, int runs, int split, bool csv, Work work)
{
    double reference;

// allocator: std::allocator, bound: compile-time
    reference = measure((SA|mask|CT), csv, 0.0,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    StdCont c;
                    c.reserve(split);
                    work(c,split);
                }});

// allocator: monotonic, bound: compile-time
    measure((MT|mask|CT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialPool bsp(pool, sizeof(pool));
                    MonoCont c(&bsp);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: monotonic, bound: compile-time, drop
    measure((MTD|mask|CT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialPool bsp(pool, sizeof(pool));
                    auto* c = new(bsp) MonoCont(&bsp);
                    c->reserve(split);
                    work(*c, split);
                }});


// allocator: multipool, bound: compile-time
    measure((PL|mask|CT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::Multipool mp;
                    MultiCont c(&mp);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: multipool, bound: compile-time, drop
    measure((PLD|mask|CT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::Multipool mp;
                    auto* c = new(mp) MultiCont(&mp);
                    c->reserve(split);
                    work(*c, split);
                }});

// allocator: multipool/monotonic, bound: compile-time
    measure((PM|mask|CT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialAllocator bsa(pool, sizeof(pool));
                    bdlma::Multipool mp(&bsa);
                    MultiCont c(&mp);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: multipool/monotonic, bound: compile-time, drop monotonic
    measure((PMD|mask|CT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialAllocator bsa(pool, sizeof(pool));
                    bdlma::Multipool* mp =
                            new(bsa) bdlma::Multipool(&bsa);
                    auto* c = new(*mp) MultiCont(mp);
                    c->reserve(split);
                    work(*c, split);
                }});


// allocator: newdelete, bound: run-time
    measure((SA|mask|RT), csv, reference,
        [runs,split,work]() {
                bslma::NewDeleteAllocator mfa;
                for (int run: range{0, runs}) {
                    PolyCont c(&mfa);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: monotonic, bound: run-time
    measure((MT|mask|RT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialAllocator bsa(pool, sizeof(pool));
                    PolyCont c(&bsa);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: monotonic, bound: run-time, drop
    measure((MTD|mask|RT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialAllocator bsa(pool, sizeof(pool));
                    auto* c = new(bsa) PolyCont(&bsa);
                    c->reserve(split);
                    work(*c, split);
                }});

// allocator: multipool, bound: run-time
    measure((PL|mask|RT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::MultipoolAllocator mpa;
                    PolyCont c(&mpa);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: multipool, bound: run-time, drop
    measure((PLD|mask|RT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::MultipoolAllocator mpa;
                    auto* c = new(mpa) PolyCont(&mpa);
                    c->reserve(split);
                    work(*c, split);
                }});

// allocator: multipool/monotonic, bound: run-time
    measure((PM|mask|RT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialAllocator bsa(pool, sizeof(pool));
                    bdlma::MultipoolAllocator mpa(&bsa);
                    PolyCont c(&mpa);
                    c.reserve(split);
                    work(c, split);
                }});

// allocator: multipool/monotonic, bound: run-time, drop monotonic
    measure((PMD|mask|RT), csv, reference,
        [runs,split,work]() {
                for (int run: range{0, runs}) {
                    bdlma::BufferedSequentialAllocator bsa(pool, sizeof(pool));
                    bdlma::MultipoolAllocator* mp =
                            new(bsa) bdlma::MultipoolAllocator(&bsa);
                    auto* c = new(*mp) PolyCont(mp);
                    c->reserve(split);
                    work(*c, split);
                }});
}

void apply_containers(int runs, int split, bool csv)
{
    apply_allocation_strategies<
            std::vector<int>,monotonic::vector<int>,
            multipool::vector<int>,poly::vector<int>>(
        VEC|INT, runs * 128, split, csv,
        [] (auto& c, int elems) {
            for (int elt: range{0, elems}) {
                c.emplace_back(elt);
                side_effect(&c.back(), 4);
        }});

    apply_allocation_strategies<
            std::vector<std::string>,
            monotonic::vector<monotonic::string>,
            multipool::vector<multipool::string>,
            poly::vector<poly::string>>(
        VEC|STR, runs * 128, split, csv,
        [] (auto& c, int elems) {
            for (int elt: range{0, elems}) {
                c.emplace_back(sptr(), slen());
                side_effect(const_cast<char*>(c.back().data()), 4);
        }});

    apply_allocation_strategies<
            std::unordered_set<int>,monotonic::unordered_set<int>,
            multipool::unordered_set<int>,poly::unordered_set<int>>(
        HASH|INT, runs * 128, split, csv,
        [] (auto& c, int elems) {
            for (int elt: range{0, elems})
                c.emplace(elt);
        });

    apply_allocation_strategies<
            std::unordered_set<std::string>,
            monotonic::unordered_set<monotonic::string>,
            multipool::unordered_set<multipool::string>,
            poly::unordered_set<poly::string>>(
        HASH|STR, runs * 128, split, csv,
        [] (auto& c, int elems) {
            for (int elt: range{0, elems})
                c.emplace(sptr(), slen());
        });

    apply_allocation_strategies<
            std::vector<std::vector<int>>,
            monotonic::vector<monotonic::vector<int>>,
            multipool::vector<multipool::vector<int>>,
            poly::vector<poly::vector<int>>>(
        VECVEC|INT, runs, split, csv,
        [] (auto& c, int elems) {
            c.emplace_back(128, 1);
            for (int elt: range{0, elems})
                c.emplace_back(c.back());
        });

    apply_allocation_strategies<
            std::vector<std::vector<std::string>>,
            monotonic::vector<monotonic::vector<monotonic::string>>,
            multipool::vector<multipool::vector<multipool::string>>,
            poly::vector<poly::vector<poly::string>>>(
        VECVEC|STR, runs, split, csv,
        [split] (auto& c, int elems) {
            typename std::decay<decltype(c)>::type::value_type s(
                c.get_allocator());
            s.reserve(128);
            for (int i : range{0,128})
                s.emplace_back(sptr(), slen());
            c.emplace_back(std::move(s));
            for (int elt: range{0, split})
                c.emplace_back(c.back());
        });

    apply_allocation_strategies<
            std::vector<std::unordered_set<int>>,
            monotonic::vector<monotonic::unordered_set<int>>,
            multipool::vector<multipool::unordered_set<int>>,
            poly::vector<poly::unordered_set<int>>>(
        VECHASH|INT, runs, split, csv,
        [split] (auto& c, int elems) {
            int in[128]; std::generate(in, in+128, random_engine);
            typename std::decay<decltype(c)>::type::value_type s(
                128, c.get_allocator());
            s.insert(in, in+128);
            c.push_back(std::move(s));
            for (int elt: range{0, split})
                c.emplace_back(c.back());
        });

    apply_allocation_strategies<
            std::vector<std::unordered_set<std::string>>,
            monotonic::vector<monotonic::unordered_set<monotonic::string>>,
            multipool::vector<multipool::unordered_set<multipool::string>>,
            poly::vector<poly::unordered_set<poly::string>>>(
        VECHASH|STR, runs, split, csv,
        [split] (auto& c, int elems) {
            typename std::decay<decltype(c)>::type::value_type s(
                128, c.get_allocator());
            for (int i : range{0,128})
                s.emplace(sptr(), slen());
            c.push_back(std::move(s));
            for (int elt: range{0, split})
                c.emplace_back(c.back());
        });

    apply_allocation_strategies<
            std::unordered_set<std::vector<int>,
                    my_hash<std::vector<int>>,
                    my_equal<std::vector<int>>>,
            monotonic::unordered_set<monotonic::vector<int>,
                    my_hash<monotonic::vector<int>>,
                    my_equal<monotonic::vector<int>>>,
            multipool::unordered_set<multipool::vector<int>,
                    my_hash<multipool::vector<int>>,
                    my_equal<multipool::vector<int>>>,
            poly::unordered_set<poly::vector<int>,
                    my_hash<poly::vector<int>>,
                    my_equal<poly::vector<int>>>>(
        HASHVEC|INT, runs, split, csv,
        [split] (auto& c, int elems) {
            typename std::decay<decltype(c)>::type::key_type s(
                c.get_allocator());
            s.reserve(128);
            for (int i: range{0, 128})
                s.emplace_back(random_engine());
            c.emplace(std::move(s));
            for (int elt: range{0, split})
                c.emplace(*c.begin());
        });

    apply_allocation_strategies<
            std::unordered_set<std::vector<std::string>,
                my_hash<std::vector<std::string>>,
                my_equal<std::vector<std::string>>>,
            monotonic::unordered_set<monotonic::vector<monotonic::string>,
                my_hash<monotonic::vector<monotonic::string>>,
                my_equal<monotonic::vector<monotonic::string>>>,
            multipool::unordered_set<multipool::vector<multipool::string>,
                my_hash<multipool::vector<multipool::string>>,
                my_equal<multipool::vector<multipool::string>>>,
            poly::unordered_set<poly::vector<poly::string>,
                my_hash<poly::vector<poly::string>>,
                my_equal<poly::vector<poly::string>>>>(
        HASHVEC|STR, runs, split, csv,
        [split] (auto& c, int elems) {
            typename std::decay<decltype(c)>::type::key_type s(
                c.get_allocator());
            s.reserve(128);
            for (int i: range{0, 128})
                s.emplace_back(sptr(), slen());
            c.emplace(std::move(s));
            for (int elt: range{0, split})
                c.emplace(*c.begin());
        });

    apply_allocation_strategies<
            std::unordered_set<std::unordered_set<int>,
                    my_hash<std::unordered_set<int>>,
                    my_equal<std::unordered_set<int>>>,
            monotonic::unordered_set<
                monotonic::unordered_set<int>,
                    my_hash<monotonic::unordered_set<int>>,
                    my_equal<monotonic::unordered_set<int>>>,
            multipool::unordered_set<
                multipool::unordered_set<int>,
                    my_hash<multipool::unordered_set<int>>,
                    my_equal<multipool::unordered_set<int>>>,
            poly::unordered_set<
                poly::unordered_set<int>,
                    my_hash<poly::unordered_set<int>>,
                    my_equal<poly::unordered_set<int>>>>(
        HASHHASH|INT, runs, split, csv,
        [split] (auto& c, int elems) {
            typename std::decay<decltype(c)>::type::key_type s(
                128, c.get_allocator());
            for (int i: range{0, 128})
                s.emplace(random_engine());
            c.emplace(std::move(s));
            for (int elt: range{0, split})
                c.emplace(*c.begin());
        });

    apply_allocation_strategies<
            std::unordered_set<std::unordered_set<std::string>,
                    my_hash<std::unordered_set<std::string>>,
                    my_equal<std::unordered_set<std::string>>>,
            monotonic::unordered_set<
                monotonic::unordered_set<monotonic::string>,
                    my_hash<monotonic::unordered_set<monotonic::string>>,
                    my_equal<monotonic::unordered_set<monotonic::string>>>,
            multipool::unordered_set<
                multipool::unordered_set<multipool::string>,
                    my_hash<multipool::unordered_set<multipool::string>>,
                    my_equal<multipool::unordered_set<multipool::string>>>,
            poly::unordered_set<
                poly::unordered_set<poly::string>,
                    my_hash<poly::unordered_set<poly::string>>,
                    my_equal<poly::unordered_set<poly::string>>>>(
        HASHHASH|STR, runs, split, csv,
        [split] (auto& c, int elems) {
            typename std::decay<decltype(c)>::type::key_type s(
                128, c.get_allocator());
            for (int i: range{0, 128})
                s.emplace(sptr(), slen());
            c.emplace(std::move(s));
            for (int elt: range{0, split})
                c.emplace(*c.begin());
        });
}


//int main(int ac, char** av)
//{
//    std::ios::sync_with_stdio(false);
//    if (ac != 3 && ac != 4)
//        usage(*av, 1);
//    int logsize = atoi(av[1]);
//    int logsplit = atoi(av[2]);
//    if (logsize < 1 || logsize > max_problem_logsize)
//        usage(*av, 2);
//    if (logsplit < 1 || logsplit > logsize)
//        usage(*av, 3);
//    bool csv = (ac == 4);
//
//    if (!csv) {
//        std::cout << "Total # of objects = 2^" << logsize
//                  << ", # elements per container = 2^" << logsplit
//                  << ", # rounds = 2^" << logsize - logsplit << "\n";
//    }
//
//    int size = 1 << logsize;
//    int split = 1 << logsplit;
//    int runs = size / split;
//
//    std::uniform_int_distribution<char> char_dist('!', '~');
//    for (char& c : trash)
//        c = char_dist(random_engine);
//
//    // The actual storage
//    memset(pool, 1, sizeof(pool));  // Fault in real memory
//
//    std::cout << std::setprecision(3);
//
//    apply_containers(runs, split, csv);
//
//    return 0;
//}

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
