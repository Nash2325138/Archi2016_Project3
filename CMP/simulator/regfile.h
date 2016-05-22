#ifndef REGFILE_H
#define REGFILE_H

#include <vector>
#include <cstdio>
#include <cstdlib>
class Registers : public std::vector< unsigned int >
{
public:
	Registers(unsigned int sp);
	int getReg(int location);
};

void regfile_function(void);
#endif