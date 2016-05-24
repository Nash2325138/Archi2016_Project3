#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>
#include <cstdio>
#include <cstdlib>

void instruction_function(void);

typedef struct CacheEntry {
	bool valid;
	bool MRU;
	int *tag;
	int *content;
	CacheEntry(int blockSize) {
		valid = MRU = false;
		int wordNumPerBlock = blockSize / 4;
		tag = new int[wordNumPerBlock];
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

typedef struct TLBEntry {
	bool valid;
	bool dirty;
	bool access;
	unsigned int tag;
	unsigned int physicalAddress; 	// at most 10 bit will be use
	TLBEntry() {
		valid = dirty = access = false;
		tag = physicalAddress = 0;
	}
}TLBEntry;

typedef struct PageTableEntry {
	bool valid;
	bool dirty;
	bool access;
	unsigned int physicalAddress;
	PageTableEntry() {
		valid = dirty = access = false;
		physicalAddress = 0;
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
	int associative;

	std::vector<TLBEntry *> TLB;
	std::vector<PageTableEntry *> pageTable;

	std::vector<CacheEntry *> cache;
	std::vector<int> memory;

	Instructions(unsigned int PC, FILE *iimage, int argc, char const *argv[]);
	~Instructions();
	//writeTo(unsigned vertualAddress, )

private:
	int log2(unsigned int target);
};

#endif
