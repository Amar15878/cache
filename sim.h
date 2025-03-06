#ifndef SIM_H
#define SIM_H

#include <vector>
#include <cstdint>
#include <string>

struct cache_params_t {
	// cache parameters
    uint32_t BLOCKSIZE;
    uint32_t L1_SIZE;
    uint32_t L1_ASSOC;
    uint32_t L2_SIZE;
    uint32_t L2_ASSOC;
    uint32_t PREF_N;
    uint32_t PREF_M;
};

class CacheLine {
public:
	// cache line parameters
    uint32_t tag;
    bool valid;
    bool dirty;

    CacheLine();
};

class Cache {
public:
	// int 32 cache parameters
    uint32_t cache_size;
    uint32_t assoc;
    uint32_t block_size;
    uint32_t nos_sets;

	// cache data in form of vector of vectors
    std::vector<std::vector<CacheLine>> cache_line_data;
    std::vector<std::vector<int>> lruOrder;
    
	// pointer to next level cache
    Cache* next_level_cache;

	// counters for cache
    uint32_t read_count;
    uint32_t read_miss_count;
    uint32_t write_count;
    uint32_t write_miss_count;
    uint32_t write_back_count;
    uint32_t memory_count;
    uint32_t l1_prefetch_count;
    uint32_t l2_read_prefetch;
    uint32_t l2_readmiss_prefetch;
    uint32_t l2_prefetch_count;

	// constructor
    Cache(uint32_t cache_size, uint32_t assoc, uint32_t block_size, Cache* next_level_cache = nullptr);

    //functions
    void request(uint32_t address, char r_or_w);
    void updateLRU(uint32_t index, int index_of_cache_line);
    void print_results(int writeBackCt) const;
	void print_results_new() const;
    void print_results_l2() const;
	static void print_default_results_l2();
    void print_contents(const std::string& name_of_cache) const;
};

#endif