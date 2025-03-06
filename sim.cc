#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include "sim.h"

// CacheLine constructor
CacheLine::CacheLine() : tag(0), valid(false), dirty(false) {}


Cache::Cache(uint32_t cache_size, uint32_t assoc, uint32_t block_size, Cache* next_level_cache)
    :cache_size(cache_size), assoc(assoc), block_size(block_size), next_level_cache(next_level_cache),
    read_count(0), read_miss_count(0), write_count(0), write_miss_count(0),
    write_back_count(0), memory_count(0), l1_prefetch_count(0),
    l2_read_prefetch(0), l2_readmiss_prefetch(0), l2_prefetch_count(0) {

    //old boring
    /*if (cache_size > 0) {
        nos_sets = cache_size / (assoc * block_size);
    }
    else {
        nos_sets = 0;
    }*/


    // new 
	nos_sets = (cache_size > 0) ? (cache_size / (assoc * block_size)) : 0;

    //
    cache_line_data.resize(nos_sets, std::vector<CacheLine>(assoc));
    
    
    lruOrder.resize(nos_sets);
    
    
    for (uint32_t i = 0; i < nos_sets; ++i) {
        lruOrder[i].resize(assoc);
        for (uint32_t j = 0; j < assoc; ++j) {
            lruOrder[i][j] = j;
        }
    }
}

// Function to handle cache requests
void Cache::request(uint32_t address, char r_or_w) {
    
	// Calculate the block offset and index bits
    int blockOffsetBits = log2(block_size);
    int indexBits = log2(nos_sets);


    // Extract the index and tag from the address
	unsigned int index = (address >> blockOffsetBits) & ((1 << indexBits) - 1); // Extract index by shifting right and masking
	unsigned int tag = address >> (blockOffsetBits + indexBits);                // Extract tag " " " " 

	
    // Initialize hit and index of cache line
    bool hit = false;
    int index_of_cache_line = -1;


    // Check if the block is already in the cache (hit or miss)
    for (uint32_t i = 0; i < assoc; ++i) {
        if (cache_line_data[index][i].valid && cache_line_data[index][i].tag == tag) {
            hit = true;
            index_of_cache_line = i;
            if (r_or_w == 'w') {
                cache_line_data[index][i].dirty = true;  // Mark dirty on write
            }
            break;
        }
    }

    if (hit) {
		// Cache hit case  -> Initialize the counters
        if (r_or_w == 'r') {
            read_count++;
        }
        else if (r_or_w == 'w') {
            write_count++;
        }

        // Update LRU order on cache hit
        updateLRU(index, index_of_cache_line);
    
    
    }
    else {
        // Cache miss
        if (r_or_w == 'r') {
            read_miss_count++;
            read_count++;
        }
        else if (r_or_w == 'w') {
            write_miss_count++;
            write_count++;
        }

        // Eviction using LRU
        int lruIndex = lruOrder[index][0]; // Least Recently Used line

        if (cache_line_data[index][lruIndex].valid) {
            // If the LRU line is valid, check if it needs to be written back
            if (cache_line_data[index][lruIndex].dirty) {
                // Handle Write-back for dirty block
                if (next_level_cache != nullptr) {
                    write_back_count++;  // Track write-back to next level
                    // Calculate the full address of the evicted block for L2
                    uint32_t evictedAddress = (cache_line_data[index][lruIndex].tag << (blockOffsetBits + indexBits)) | (index << blockOffsetBits);
                    next_level_cache->request(evictedAddress, 'w');  // Write back to next level (L2)
                }
                else {
                    // Write-back to memory if no next level
                    write_back_count++;   // Track write-back to memory
                    memory_count++;       // Track memory traffic on write-back
                }
            }

            // Fetch the new block from the next level or memory
            if (next_level_cache != nullptr) {
                next_level_cache->request(address, 'r');
            }
            else {
                memory_count++;  // Track memory traffic on read from memory
            }
        }
        else {
            // LRU line is invalid, just replace without eviction
            if (next_level_cache != nullptr) {
                next_level_cache->request(address, 'r');
            }
            else {
                memory_count++;  // Track memory traffic on read from memory
            }
        }

        // Replace the least recently used line with new block
        cache_line_data[index][lruIndex].tag = tag;
        cache_line_data[index][lruIndex].valid = true;
        cache_line_data[index][lruIndex].dirty = (r_or_w == 'w');  // Mark dirty if it's a write

        // Update the LRU order after replacement
        updateLRU(index, lruIndex);
    }
}

// Function to update the LRU order
void Cache::updateLRU(uint32_t index, int index_of_cache_line) {
    
    // Find the current position of the cache line in the LRU list
    auto it = std::find(lruOrder[index].begin(), lruOrder[index].end(), index_of_cache_line);
    
    if (it != lruOrder[index].end()) {
        
        // Remove it from its current position
        lruOrder[index].erase(it);
        
        
        // Push it to the back (most recently used)
        lruOrder[index].push_back(index_of_cache_line);
    }
}


// Og v2 : Print the L1 results
void Cache::print_results_new() const {
    std::cout << std::dec;
    std::cout << std::endl << "===== Measurements =====\n";
    std::cout << "a. L1 reads:                   " << read_count << "\n";
    std::cout << "b. L1 read misses:             " << read_miss_count << "\n";
    std::cout << "c. L1 writes:                  " << write_count << "\n";
    std::cout << "d. L1 write misses:            " << write_miss_count << "\n";
    std::cout << "e. L1 miss rate:               " << std::fixed << std::setprecision(4) << (float)(read_miss_count + write_miss_count) /(read_count + write_count) << "\n";
    std::cout << "f. L1 writebacks:              " << write_back_count << "\n";
    std::cout << "g. L1 prefetches:              " << l1_prefetch_count << "\n";
}

// Function to print the L2 results when L2 cache is present
void Cache::print_results_l2() const {
    std::cout << "h. L2 reads (demand):          " << std::dec << read_count << "\n";
    std::cout << "i. L2 read misses (demand):    " << read_miss_count << "\n";
    std::cout << "j. L2 reads (prefetch):        " << l2_read_prefetch << "\n";
    std::cout << "k. L2 read misses (prefetch):  " << l2_readmiss_prefetch << "\n";
    std::cout << "l. L2 writes:                  " << write_count << "\n";
    std::cout << "m. L2 write misses:            " << write_miss_count << "\n";
    std::cout << "n. L2 miss rate:               " << std::fixed << std::setprecision(4) << (float)(read_miss_count) / (read_count) << "\n";
    std::cout << "o. L2 writebacks:              " << write_back_count << "\n";
    std::cout << "p. L2 prefetches:              " << l2_prefetch_count << "\n";
    std::cout << "q. memory traffic:             " << memory_count << std::endl;
}

// Function to print the default L2 results when L2 cache is not present
void Cache::print_default_results_l2() {
    std::cout << "h. L2 reads (demand):          0\n";
    std::cout << "i. L2 read misses (demand):    0\n";
    std::cout << "j. L2 reads (prefetch):        0\n";
    std::cout << "k. L2 read misses (prefetch):  0\n";
    std::cout << "l. L2 writes:                  0\n";
    std::cout << "m. L2 write misses:            0\n";
    std::cout << "n. L2 miss rate:               0.0000\n";
    std::cout << "o. L2 writebacks:              0\n";
    std::cout << "p. L2 prefetches:              0\n";
}

// Function to print the contents of the cache
void Cache::print_contents(const std::string& name_of_cache) const {
    std::cout << std::endl << "===== " << name_of_cache << " contents =====\n";

    for (size_t i = 0; i < cache_line_data.size(); ++i) {
        // Adjust set number width
        std::cout << "set " << std::setw(6) << std::dec << i << ": ";

        // Print lines from MRU to LRU
        for (int j = assoc - 1; j >= 0; --j) {
            int index_of_cache_line = lruOrder[i][j];  // Get the index from the LRU order

            if (cache_line_data[i][index_of_cache_line].valid) {
                std::cout << std::hex << std::setw(8) << cache_line_data[i][index_of_cache_line].tag
                // Print tag + 'D' if dirty, adjust width for uniform spacing
                    << (cache_line_data[i][index_of_cache_line].dirty ? " D" : "  ");
            }
            else {
                // Adjust space for invalid lines
                std::cout << std::setw(12) << " ";
            }
        }
        std::cout << "\n";
    }
}

//Main function to simulate the cache
int main(int argc, char* argv[]) {
    FILE* fp;
    char* trace_file;
    cache_params_t params;
    char rw;
    uint32_t addr;

    if (argc != 9) {
        printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
        exit(EXIT_FAILURE);
    }

    params.BLOCKSIZE = (uint32_t)atoi(argv[1]);
    params.L1_SIZE = (uint32_t)atoi(argv[2]);
    params.L1_ASSOC = (uint32_t)atoi(argv[3]);
    params.L2_SIZE = (uint32_t)atoi(argv[4]);
    params.L2_ASSOC = (uint32_t)atoi(argv[5]);
    params.PREF_N = (uint32_t)atoi(argv[6]);
    params.PREF_M = (uint32_t)atoi(argv[7]);
    trace_file = argv[8];

    // Open the trace file for reading.
    fp = fopen(trace_file, "r");
    if (fp == (FILE*)NULL) {
        // Exit with an error if file open failed.
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    // Simulator configuration output
    printf("===== Simulator configuration =====\n");
    printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
    printf("L1_SIZE:    %u\n", params.L1_SIZE);
    printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
    printf("L2_SIZE:    %u\n", params.L2_SIZE);
    printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
    printf("PREF_N:     %u\n", params.PREF_N);
    printf("PREF_M:     %u\n", params.PREF_M);
    printf("trace_file: %s\n", trace_file);

	// Create L2 cache object if L2 cache size is greater than 0 then create L2 cache object
    Cache* l2_cache = nullptr;
    if (params.L2_SIZE > 0) {
        l2_cache = new Cache(params.L2_SIZE, params.L2_ASSOC, params.BLOCKSIZE);
    }

	// Create L1 cache object
    Cache l1_cache(params.L1_SIZE, params.L1_ASSOC, params.BLOCKSIZE, l2_cache);

    // Process each trace file request
    while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {    // Stay in the loop if fscanf() successfully parsed two tokens as specified.
        l1_cache.request(addr, rw);
    }
	//print the contents of the cache
    l1_cache.print_contents("L1");
    if (l2_cache != nullptr) {
        l2_cache->print_contents("L2");
    }

	// Print the results
    l1_cache.print_results_new();
    if (l2_cache != nullptr) {
        l2_cache->print_results_l2();
    }
    else {
        Cache::print_default_results_l2();
		std::cout << "q. memory traffic:             " << l1_cache.memory_count << std::endl;
    }

    fclose(fp);
    return 0;
}