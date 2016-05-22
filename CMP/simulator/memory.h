#ifndef MEMORY_H
#define MEMORY_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
void memory_function(void);

class Memory : public std::vector<unsigned int>
{
public:
	Memory(FILE *dimage);
};
#endif
