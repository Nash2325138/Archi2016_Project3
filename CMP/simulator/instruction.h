#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>
#include <cstdio>
#include <cstdlib>

void instruction_function(void);

typedef struct CacheEntry {
	bool valid;
	bool MRU;
	unsigned int *tag;
	int *content;
	CacheEntry(int blockSize) {
		valid = MRU = false;
		int wordNumPerBlock = blockSize / 4;
		tag = new unsigned int[wordNumPerBlock];
		content = new int[wordNumPerBlock];
		for(int i=0 ; i<wordNumPerBlock ; i++) {
			tag[i] = content[i] = 0;
		}
	}
	~CacheEntry() {
		delete tag;
		delete content;
	}
}CacheEntry;

typedef struct MemoryEntry {
	bool available;
	int lastUsedCycle;
	int *content;
	MemoryEntry(int pageSize) {
		available = true;
		// we don't care last used cycle when initialization (all memory are available)
		int wordsPerPage = pageSize / 4;
		content = new int[wordsPerPage];
		for(int i=0 ; i<wordsPerPage ; i++) {
			content[i] = 0;
		}
	}
}MemoryEntry;

typedef struct TLBEntry {
	bool valid;
	bool dirty;
	bool access;
	int lastUsedCycle;
	unsigned int tag;
	unsigned int ppn; 	// at most 10 bit will be used
	TLBEntry() {
		valid = dirty = access = false;
		tag = ppn = 0;
	}
}TLBEntry;

typedef struct PageTableEntry {
	bool valid;
	bool dirty;
	bool access;
	unsigned int ppn;
	PageTableEntry() {
		valid = dirty = access = false;
		ppn = 0;
	}
}PageTableEntry;

class Instructions
{
public:
	std::vector<unsigned int> disk;
	
	int memorySize;
	int pageSize;
	int cacheSize;
	int blockSize;
	int associative;	// for cache, and TLB will always be fully-associative

	int pageOffsetWidth;

	int blockOffsetWidth;
	int cache_indexWidth;
	int cache_tagWidth;

	std::vector<TLBEntry *> TLB;
	std::vector<PageTableEntry *> pageTable;

	std::vector<CacheEntry *> cache;
	std::vector<MemoryEntry *> memory;

	Instructions(unsigned int PC, FILE *iimage, int argc, char const *argv[]);
	~Instructions();
	//writeTo(unsigned vertualAddress, )

	int TLB_hit;
	int TLB_miss;
	int pageTable_hit;
	int pageTable_miss;
	int cache_hit;
	int cache_miss;

	unsigned int getPAddr(unsigned int vAddr, int cycle);
	void updateTLB(unsigned int tag, unsigned int ppn, int cycle);

	// return the swapped ppn
	// (no need to update swapped memory entry's lastUsedCycle, just let cache/memory do this)
	unsigned int swap_writeBack(unsigned int vAddr);

private:
	int log2(unsigned int target);
};

#endif
