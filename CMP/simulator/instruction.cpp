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

	int cacheEntryNum = cacheSize / blockSize;
	cache.resize(cacheEntryNum);
	for(int i=0 ; i<cacheEntryNum ; i++) {
		cache[i] = new CacheEntry(blockSize);
	}
	memory.resize(memorySize);
	for(int i=0 ; i<memorySize ; i++) {
		memory[i] = 0;
	}

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