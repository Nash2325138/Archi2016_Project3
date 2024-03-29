#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>
#include <cstdio>
#include <cstdlib>

void instruction_function(void);


extern void write_32bits_to_image(FILE *image, unsigned int number);
extern void print_dissembled_inst(unsigned int inst);


class Instructions
{
private:
	class CacheEntry {
	public:
		bool valid;
		bool MRU;
		unsigned int tag;
		int *content;
		CacheEntry(int blockSize) {
			valid = MRU = false;
			tag = 0;
			int wordNumPerBlock = blockSize / 4;
			content = new int[wordNumPerBlock];
			for(int i=0 ; i<wordNumPerBlock ; i++) {
				content[i] = 0;
			}
		}
		~CacheEntry() {
			delete content;
		}
	};

	class MemoryEntry {
	public:
		bool available;
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
	};

	class TLBEntry {
	public:
		bool valid;
		int lastUsedCycle;
		unsigned int tag;
		unsigned int ppn; 	// at most 10 bit will be used
		TLBEntry() {
			valid = false;
			tag = ppn = 0;
		}
	};

	class PageTableEntry {
	public:
		bool valid;
		unsigned int ppn;
		int ppn_lastUsedCycle;
		PageTableEntry() {
			valid = false;
			ppn = 0;
		}
	};
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
	int cache_setNum;

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

	unsigned int getDataByVaddr(unsigned int vAddr, int cycle);
	unsigned int getPAddr(unsigned int vAddr, int cycle);
	void updateTLB(unsigned int tag, unsigned int ppn, int cycle);
	void updateCache(unsigned int pAddr, unsigned int index, unsigned int tag, unsigned int blockOffset);
	void print_TLB();
	void print_cache();
	void print_memory();
	void print_pageTable();
	// return the swapped ppn
	// (no need to update swapped memory entry's lastUsedCycle, just let cache/memory do this)
	unsigned int swap_writeBack(unsigned int vAddr);
	void setCacheMRU(unsigned int index, unsigned int posInSet);

private:
	int log2(unsigned int target);
};

#endif
