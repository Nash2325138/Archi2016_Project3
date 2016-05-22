#include "regfile.h"

void regfile_function(void)
{
	printf("This is regfile_function() in regfile.cpp\n");
}


Registers::Registers(unsigned int sp) : std::vector< unsigned int >(32, 0)
{
	this->at(29) = sp;
}