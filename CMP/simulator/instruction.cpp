#include "./instruction.h"
#include <iterator>
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
	if(argc == 1) {
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
	cache_tagWidth = 32 - blockOffsetWidth - cache_indexWidth;
	cache_setNum = cacheEntryNum / associative;
	cache_indexWidth = this->log2(cache_setNum);


	TLB_hit = TLB_miss = pageTable_hit = pageTable_miss = cache_hit = cache_miss = 0;
}

unsigned int Instructions::getDataByVaddr(unsigned int vAddr, int cycle)
{
	unsigned int pAddr = this->getPAddr(vAddr, cycle);

	// first find this pAddr in cache
	unsigned int index = (pAddr >> blockOffsetWidth) % cache_setNum; // get index of the set
	unsigned int tag = pAddr >> (blockOffsetWidth + cache_indexWidth);
	unsigned int blockOffset = pAddr % blockSize;
	for(int i=0 ; i<associative ; i++) {
		CacheEntry *target = cache.at(index * cache_setNum + i);
		if( target->valid == false ) continue;
		if( target->tag == tag) {
			cache_hit++;
			return target->content[blockOffset >> 2];
		}
	}

	// No return implies cache miss
	cache_miss++;
	// Then we need to find the content in memory
	unsigned int ppn = pAddr >> pageOffsetWidth;
	unsigned int pageOffset = pAddr % pageSize;

	// get the content directly from memory and update lastUsedCycle ( a bit tricky )
	int newContent = memory.at(ppn)->content[pageOffset / 4];
	
	// According to discussion on ilms, I move memory page's last used cycle from memory entry to page table
	//memory.at(ppn)->lastUsedCycle = cycle;

	updateCache(pAddr);

	return newContent;
}

void Instructions::updateCache(unsigned int pAddr)
{
	// if there's empty entry in the set, just place data to that entry from memory (with a block of data)
	// and handle the MRU bit


	// if there isn't, find the Bit-Pseudo LRU entry

	// write back the Bit-Pseudo LRU entry to memory

	// place data to the replaced entry and handle MRU bit
}
unsigned int Instructions::getPAddr(unsigned int vAddr, int cycle)
{
	unsigned int tag = vAddr >> pageOffsetWidth;
	// BTW, tag == virtual page number when TLB is fully-associative
	
	unsigned int pageOffset = vAddr % pageSize;
	unsigned int pAddr;
	// search tag in fully-associative TLB
	for(unsigned int i=0 , size = TLB.size() ; i<size ; i++) {
		if(TLB[i]->valid == false) continue;
		if(TLB[i]->tag == tag) {
			TLB_hit++;
			pAddr = (TLB[i]->ppn << pageOffsetWidth ) | pageOffset;
			TLB[i]->lastUsedCycle = cycle;
			
			// According to discussion on ilms, also update page table's ppn_lastUsedCycle when TLB hit
			pageTable.at(tag)->ppn_lastUsedCycle = cycle;
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
		pageTable[tag]->ppn_lastUsedCycle = cycle;

		// and we need to do: update TLB
		this->updateTLB(tag, pageTable[tag]->ppn, cycle);
		return pAddr;
	} 
	else
	{
		// if not valid, then the data can only be found in disk
		pageTable_miss++;
		// then need to do: Swap / update PageTable / update TLB

		// Swap
		unsigned int replaced_ppn = this->swap_writeBack(vAddr);

		// update the swapped-in page in page table
		pageTable[tag]->valid = true;
		pageTable[tag]->ppn = replaced_ppn;
		pageTable[tag]->ppn_lastUsedCycle = cycle;

		// update TLB
		this->updateTLB(tag, replaced_ppn, cycle);
		pAddr = (replaced_ppn << pageOffsetWidth) | pageOffset;
		return pAddr;
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
	// If memory space is available, place data to the first available page closest to the page #0
	int i, size;
	for(i=0 , size = memory.size() ; i < size ; i++) {
		if(memory[i]->available) {
			// just place data to available page
			memory[i]->available = false;
			unsigned int disk_startAddr;
			disk_startAddr = (vAddr >> pageOffsetWidth) << pageOffsetWidth;
			for(int j=0 ; j < pageSize >> 2 ; j++) {
				memory[i]->content[j] = disk[disk_startAddr + j];
			}
			break;
		}
	}
	if( i < size ) return i;

	// find the LRU page in memory. Pick the least indexed page to be the victim in case of tie
	//****  Use "page table" to find the LRU page in memory!! Then we can know the replaced page's disk address  ****//
	int min = -1;
	std::vector<PageTableEntry *>::iterator iter, end, chosen;
	for(iter = chosen = pageTable.begin() , end = pageTable.end() ; iter != end ; iter++) {
		if( (*iter)->valid == false ) continue;
		if( min == -1 || (*iter)->ppn_lastUsedCycle < min ) {
			min = (*iter)->ppn_lastUsedCycle;
			chosen = iter;
		}
	}
	// write back the LRU page to disk ( swap out )
	unsigned int replaced_vpn = std::distance(pageTable.begin(), chosen) - 1;
	unsigned int disk_startAddr = replaced_vpn << pageOffsetWidth;
	for(int j=0, wordNum = pageSize << 2 ; j < wordNum ; j++) {
		disk.at(disk_startAddr + j) = memory.at( (*chosen)->ppn )->content[j];
	}
	// update the replaced vpn's valid bit in page table
	(*chosen)->valid = false;
	// update the replaced vpn's valid bit in TLB if it's in TLB
	for(std::vector<TLBEntry *>::iterator iter=TLB.begin(), end = TLB.end() ; iter != end ; iter++) {
		if((*iter)->tag == replaced_vpn) (*iter)->valid = 0;
	}
	// update the replaced blocks' valid bit in cache if it's in cache
	// (those blocks which is in the replaced page)
	/* may be unnecessary */
	unsigned int replaced_pageAddr = ( (*chosen)->ppn << pageOffsetWidth );
	for(int i=0 ; i<cache_setNum ; i++) {
		for(int j=0 ; j<associative ; j++) {
			CacheEntry *target = cache.at(i * cache_setNum + j);
			// rebuild the block's addr
			unsigned int rebuilt_blockAddr = target->tag << cache_indexWidth;
			rebuilt_blockAddr |= i; // set id is index of cache
			rebuilt_blockAddr <<= blockOffsetWidth;

			// determine whether this is block is in the replaced page
			if( rebuilt_blockAddr / pageSize == replaced_pageAddr ) {
				// if it is in the replaced page, update its valid bit
				target->valid = 0;
			}
		}
	}

	// swap the one we want into memory
	disk_startAddr = (vAddr >> pageOffsetWidth) << pageOffsetWidth;
	for(int j=0 , wordNum = pageSize >> 2 ; j < wordNum ; j++) {
		memory.at( (*chosen)->ppn )->content[j] = disk[disk_startAddr + j];
	}

	// return the ppn we just swap into memory
	return (*chosen)->ppn;
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