#include "./memory.h"
void memory_function(void)
{
	printf("This is memory_function() in memory.cpp\n");
}

Memory::Memory(FILE *dimage) : std::vector<unsigned int>(256, 0)
{
	std::vector<unsigned int>::iterator it = this->begin();

	unsigned int num=0;
	unsigned char readByte;
	for(int i=0 ; i<4 ; i++){
		fread(&readByte, sizeof(unsigned char), 1, dimage);
		num <<= 8;
		num |= readByte;
	}
	
	for(unsigned int i=0 ; i<num ; i++){
		unsigned int value = 0;
		for(int j=0 ; j<4 ; j++){
			fread(&readByte, sizeof(unsigned char), 1, dimage);
			value <<= 8;
			value |= readByte;
		}
		*it = value;
		it++;
	}
}
