#include "./instruction.h"
#include <cstdio>

void instruction_function(void) 
{
	printf("This is instruction_function() in instruction.cpp\n");
}


InstructionMemery::InstructionMemery(unsigned int PC, FILE *iimage) : std::vector<unsigned int>(256, 0)
{
	std::vector<unsigned int>::iterator it = this->begin();
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
}