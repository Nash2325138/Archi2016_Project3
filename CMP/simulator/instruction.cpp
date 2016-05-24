#include "./instruction.h"
#include <cstdio>

void instruction_function(void) 
{
	printf("This is instruction_function() in instruction.cpp\n");
}


Instructions::Instructions(unsigned int PC, FILE *iimage, int argc, char const *argv[]) : disk(256, 0)
{
	std::vector<unsigned int>::iterator it = this->disk.begin();
	it += (PC/4);

	unsigned int num=0;
	unsigned char readByte;
	for(int i=0 ; i<4 ; i++){
		fread(&readByte, sizeof(unsigned char), 1, iimage);
		num <<= 8;
		num |= readByte;
	}

	for(unsigned int i=0 ; i<num ; i++){
		unsigned int value = 0;
		for(int j=0 ; j<4 ; j++){
			fread(&readByte, sizeof(unsigned char), 1, iimage);
			value <<= 8;
			value |= readByte;
		}
		*it = value;
		it++;
	}

	/*	
	int memorySize;
	int pageSize;
	int cacheSize;
	int blockSize;
	int associative;*/
	if(argc == 0) {
		memorySize = 64;
		pageSize = 8;
		cacheSize = 16;
		blockSize = 4;
		associative = 4;
	} else {
		memorySize = atoi(argv[0]);
		pageSize = atoi(argv[2]);
		cacheSize = atoi(argv[4]);
		blockSize = atoi(argv[5]);
		associative = atoi(argv[6]);
	}

	int pageTableSize = 1024/pageSize;
	int TLBSize = pageTableSize/4;
	pageTable.resize(pageTableSize);
	TLB.resize(TLBSize);
	for(int i=0 ; i<pageTableSize ; i++) {
		pageTable[i] = new PageTableEntry();
	}
	for(int i=0 ; i<TLBSize ; i++) {
		TLB[i] = new TLBEntry();
	}
	pageOffsetWidth = this->log2(pageSize);

	int cacheEntryNum = cacheSize / blockSize;
	cache.resize(cacheEntryNum);
	for(int i=0 ; i<cacheEntryNum ; i++) {
		cache[i] = new CacheEntry(blockSize);
	}
	memory.resize(memorySize/pageSize);
	for(unsigned int i=0, size = memory.size(); i<size ; i++) {
		memory[i] = new MemoryEntry(pageSize);
	}
	blockOffsetWidth = this->log2(blockSize);
	cache_indexWidth = this->log2(cacheEntryNum);
	cache_tagWidth = 32 - blockOffsetWidth - cache_indexWidth;


	TLB_hit = TLB_miss = pageTable_hit = pageTable_miss = cache_hit = cache_miss = 0;
}
unsigned int Instructions::getPAddr(unsigned int vAddr, int cycle)
{
	unsigned int tag = vAddr >> pageOffsetWidth;
	// BTW, tag == virtual page number when TLB is fully-associative
	
	unsigned int pageOffset = ( vAddr << (32 - pageOffsetWidth) ) >> (32 - pageOffsetWidth);
	unsigned int pAddr;
	// search tag in fully-associative TLB
	for(unsigned int i=0 , size = TLB.size() ; i<size ; i++) {
		if(TLB[i]->valid == false) continue;
		if(TLB[i]->tag == tag) {
			TLB_hit++;
			pAddr = (TLB[i]->ppn << pageOffsetWidth ) | pageOffset;
			TLB[i]->lastUsedCycle = cycle;
			return pAddr;
		}
	}
	// Every entry doesn't match tag implies TLB miss.
	// Then we need to get pAddr from page table.
	TLB_miss++;
	if(pageTable.at(tag)->valid)	// tag == vpn when TLB is fully-associative
	{	
		// if valid, then the data is in memory (pAddr exist)
		pageTable_hit++;
		pAddr = (pageTable[tag]->ppn << pageOffsetWidth) | pageOffset;
		
		// and we need to do: update TLB
		this->updateTLB(tag, pageTable[tag]->ppn, cycle);
		return pAddr;
	} 
	else
	{
		// if not valid, then the data can only be found in disk
		pageTable_miss++;
		// then need to do: Swap / update PageTable / update TLB

	}
}

void Instructions::updateTLB(unsigned int tag, unsigned int ppn, int cycle)
{
	std::vector<TLBEntry *>::iterator iter, end;
	for(iter = TLB.begin() , end = TLB.end() ; iter != end ; iter++) {
		if( (*iter)->valid == false) {
			// replace the least indexed invalid entry if exists
			(*iter)->valid = true;
			(*iter)->lastUsedCycle = cycle;
			(*iter)->tag = tag;
			(*iter)->ppn = ppn;
			break;
		}
	}
	if(iter == end) {
		// this "if" implies no invalid entry exists
		// then find out the LRU entry.
		int min = TLB.at(0)->lastUsedCycle;
		int chosen = 0;
		for(int i=1 , size = TLB.size() ; i<size ; i++) {
			if( TLB[i]->lastUsedCycle < min) {
				min = TLB[i]->lastUsedCycle;
				chosen = i;
			}
		}
		// and replace the LRU entry
		TLBEntry *target = TLB[chosen];
		target->tag = tag;
		target->ppn = ppn;
		target->lastUsedCycle = cycle;
	}
}

unsigned int Instructions::swap_writeBack(unsigned int vAddr)
{
	return -1;
}

// only when target is 2 to the power N, N >= 1, can this function give the correct answer
int Instructions::log2(unsigned int target)
{
	int ans;
	for(ans = -1 ; ans < 32 ; ans ++) {
		if(target == 0) break;
		target >>= 1;
		ans++;
	}
	return ans;
}

Instructions::~Instructions()
{
	for(unsigned int i=0 ; i<TLB.size() ; i++) delete TLB[i];
	for(unsigned int i=0 ; i<pageTable.size() ; i++) delete pageTable[i];
	for(unsigned int i=0 ; i<cache.size() ; i++) delete cache[i];
}