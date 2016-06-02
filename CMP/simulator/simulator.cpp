#define debug(fmt, ...) printf(fmt, __VA_ARGS__) 

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include "./instruction.h"
#include "./regfile.h"
#include "./memory.h"

#define DEBUG_CYCLE 999999

FILE *snapshot;
FILE *error_dump;

Memory* data;
Instructions* instructions;
Registers* regs;
unsigned int PC;
int cycle;

std::vector<unsigned int>* readImage(FILE *);
void readInput_initialize(int argc, char const *argv[]);
void print_snapshot(void);
void print_report(void);
int execute(void);
void destroy_all(void);
void sumOverflow(int aluValue1, int aluValue2);

void write_32bits_to_image(FILE *image, unsigned int number)
{
	for(int i=0 ; i<4 ; i++){
		unsigned char temp = (unsigned char)(number >> 24);
		fwrite(&temp, sizeof(unsigned char), 1, image);
		number <<= 8;
	}
}
void print_dissembled_inst(unsigned int inst)
{
	FILE *tempBin = fopen("inst_temp.bin", "wb");
	write_32bits_to_image(tempBin, 0);
	write_32bits_to_image(tempBin, 1);
	write_32bits_to_image(tempBin, inst);
	fclose(tempBin);

	system("../liitle-bird-assembler/assembler -d inst_temp.bin -o inst_temp_dissembled.txt -nolabel");
	
	FILE *tempTxt = fopen("inst_temp_dissembled.txt", "r");
	char str[200];
	fgets(str, sizeof str, tempTxt);
	str[strlen(str)-1] = '\0';
	printf("%-15s", str);
	fclose(tempTxt);
}

int main(int argc, char const *argv[])
{
	setbuf(stdout, NULL);
	if( argc != 1 && argc != 11 ) {
		fprintf(stderr, "You need to use 0 argument or 10 arguments\n");
		exit(EXIT_FAILURE);
	}
	readInput_initialize(argc, argv);
	//for(int i=0 ; i<instructions->size() ; i++)printf("%x ", instructions->at(i));
	//for(int i=0 ; i<regs->size() ; i++) printf("%x ", regs->at(i));
	//printf("\n") ;
	cycle = 0;
	snapshot = fopen("snapshot.rpt", "w");
	if(snapshot==NULL){
		fputs("snapshot write error", stderr);
		exit(666);
	}
	error_dump = fopen("error_dump.rpt", "w");
	if(error_dump==NULL){
		fputs("error_dump write error", stderr);
		exit(666);
	}


	do{
		print_snapshot();
		cycle++;
	}while (execute() != -1 && cycle < 510000);
	
	print_report();

	destroy_all();
	return 0;
}

void readInput_initialize(int argc, char const *argv[])
{
	FILE *dimage = fopen("dimage.bin", "rb");
	if( dimage==NULL ) {
		fputs("dimage read error", stderr);
		exit(2);
	}
	unsigned int sp=0;
	unsigned char readByte;
	for(int i=0 ; i<4 ; i++){
		fread(&readByte, sizeof(unsigned char), 1, dimage);
		sp <<= 8;
		sp |= readByte;
	}
	data = new Memory(dimage, argc, argv);
	//for(unsigned int i=0 ; i<data->size() ; i++)	printf("%x\n", data->at(i));

	
	FILE *iimage = fopen("iimage.bin", "rb");
	if(iimage==NULL) {
		fputs("iimage read error", stderr);
		exit(2);
	}
	PC = 0;
	for(int i=0 ; i<4; i++){
		fread(&readByte, sizeof(unsigned char), 1, iimage);
		PC <<= 8;
		PC |= readByte;
	}
	instructions = new Instructions(PC, iimage, argc, argv);
	//for(unsigned int i=0 ; i<instructions->size() ; i++)	printf("%x\n", instructions->at(i));

	regs = new Registers(sp);
	//for(unsigned int i=0 ; i<regs->size() ; i++)	printf("%d: %x\n", i, regs->at(i));
	//printf("\n");
}

int execute(void)
{
	int toReturn = 0;
	if(cycle==DEBUG_CYCLE) printf("PC==%d, ", PC);
	unsigned int inst;
	unsigned int anotherInst;
	inst = instructions->disk[PC/4];
	anotherInst = instructions->getDataByVaddr(PC, cycle);
	
	if(false) {
		printf("    cycle %3d: ", cycle);
		print_dissembled_inst(anotherInst);
		printf(", ");
		print_dissembled_inst(inst);
		printf("\n\n");
		//instructions->print_TLB();
	}

	PC += 4;
	//printf("inst==%x  ", inst);
	unsigned char opcode = (unsigned char) (inst >> 26);		//warning: unsigned char has 8 bits
	unsigned char rs =  (unsigned char) ( (inst >> 21) & 0x1f );
	unsigned char rt = (unsigned char) ( (inst >> 16) & 0x1f );
	unsigned char shamt = (unsigned char) ( (inst >> 6) & 0x1f );
	unsigned char funct = (unsigned char) (inst & 0x3f);
	unsigned char rd = (unsigned char) ( (inst >> 11) & 0x1f );
	if(cycle==DEBUG_CYCLE) printf("cycle==%d, opcode==%02hhx, inst==%08x, funct==%02x\n", cycle, opcode, inst, funct);

	if(opcode == 0x00){
		int aluValue1, aluValue2;
		if(funct != 0x08 && rd==0){								// 0x08 means jr
			if(funct == 0x00 && rt==0 && rd==0 && shamt==0 ) {}		// sll $0, $0, 0 is a NOP
			else fprintf(error_dump, "In cycle %d: Write $0 Error\n", cycle);
			toReturn = 1;

		}
		if(cycle==DEBUG_CYCLE){
			printf("rd==%d, rs==%d, rt==%d, shamt==%d\n\n", rd, rs, rt, shamt);
		}
		switch(funct)
		{

			case 0x20:	// add
				aluValue1 = (int)regs->at(rs);
				aluValue2 = (int)regs->at(rt);
				sumOverflow(aluValue1, aluValue2);
				if(toReturn!=0) return toReturn;
				regs->at(rd) = ((signed int)regs->at(rs) + (signed int)regs->at(rt)) ;
				break;

			case 0x21:	// addu
				if(toReturn!=0) return toReturn;
				regs->at(rd) = (unsigned int)regs->at(rs) + (unsigned int)regs->at(rt);
				break;

			case 0x22:	// sub
				aluValue1 = (int)regs->at(rs);
				aluValue2 = (int)regs->at(rt);
				aluValue2 = (~(aluValue2))+1;
				sumOverflow(aluValue1, aluValue2);
				if(toReturn!=0) return toReturn;
				
				regs->at(rd) = (signed int)regs->at(rs) - (signed int)regs->at(rt);
				break;

			case 0x24:	// and
				if(toReturn!=0) return toReturn;
				regs->at(rd) = regs->at(rs) & regs->at(rt);
				break;

			case 0x25:	// or
				if(toReturn!=0) return toReturn;
				regs->at(rd) = regs->at(rs) | regs->at(rt);
				break;

			case 0x26:	//xor
				if(toReturn!=0) return toReturn;
				regs->at(rd) = regs->at(rs) ^ regs->at(rt);
				break;

			case 0x27:	//nor
				if(toReturn!=0) return toReturn;
				regs->at(rd) = ~(regs->at(rs) | regs->at(rt));
				break;

			case 0x28:	//nand
				if(toReturn!=0) return toReturn;
				regs->at(rd) = ~(regs->at(rs) & regs->at(rt));
				break;

			case 0x2a:	//slt
				if(toReturn!=0) return toReturn;
				regs->at(rd) = ( (int)regs->at(rs) < (int)regs->at(rt) ) ? 1:0;
				break;

			case 0x00:	//sll
				if(toReturn!=0) return toReturn;
				regs->at(rd) = regs->at(rt) << shamt;
				break;

			case 0x02:	//srl
				if(toReturn!=0) return toReturn;
				regs->at(rd) = ((unsigned int)regs->at(rt)) >> shamt;
				break;

			case 0x03:	//sra
				if(toReturn!=0) return toReturn;
				regs->at(rd) = ((int)regs->at(rt)) >> shamt;
				break;

			case 0x08:	//jr
				if(toReturn!=0) return toReturn;
				PC = regs->at((unsigned int)rs);
				break;

			default: break;
		}
	} else {
		unsigned short immediate = inst & 0xffff;
		unsigned int address = inst & 0x3ffffff;
		unsigned int location = (regs->at(rs) + ((signed short)immediate) );;
		signed short halfLoaded;
		signed char byteLoaded;
		unsigned int tempValue;
		int aluValue1 = (int)regs->at(rs);
		int aluValue2 = (signed short)immediate;
		if(cycle==DEBUG_CYCLE){
			printf("rs==%d, rt==%d, immediate==%d\n\n", rs, rt, immediate);
		}
		if(rt==0){
			if(opcode!=0x2B && opcode!=0x29 && opcode!=0x28 && opcode!=0x04 && opcode!=0x05 && opcode!=0x07){
				if(opcode!=0x02 && opcode!=0x03 && opcode!=0x3F){
					fprintf(error_dump, "In cycle %d: Write $0 Error\n", cycle);
					toReturn = 1;
				}
			}		
		}
		switch(opcode)
		{
			//printf("immediate==%d\n", immediate);
			//--------------------------- I type start -----------------------------//
			case 0x08: 	// addi
				sumOverflow(aluValue1, aluValue2);
				if(toReturn!=0) return toReturn;
				regs->at(rt) = regs->at(rs) + ((signed short)immediate);
				break;

			case 0x09:	// addiu
				if(toReturn!=0) return toReturn;
				regs->at(rt) = regs->at(rs) + ((signed short)immediate);
				break;

			case 0x23:	//lw
				sumOverflow(aluValue1, aluValue2);
				if ( location >1020 ) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if( location % 4 != 0 ){
					fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;

				regs->at(rt) = data->at( location/4 );
				data->getDataByVaddr(location, cycle);
				break;

			case 0x21:	//lh
				sumOverflow(aluValue1, aluValue2);
				if( location > 1022) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if( location % 2 != 0){
					fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;

				if(location%4==0) halfLoaded = (signed short) ( (data->at(location/4)) >> 16);
				else if(location%2==0) halfLoaded = (signed short) ( (data->at(location/4)) & 0x0000ffff );
				data->getDataByVaddr(location, cycle);
				regs->at(rt) = (signed short)halfLoaded;		// <-------- this line is very important!!!
				break;

			case 0x25:	//lhu 
				sumOverflow(aluValue1, aluValue2);
				if( location > 1022) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if( location % 2 != 0){
					fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;

				if(location%4==0) halfLoaded = (unsigned short) ( ((unsigned int)(data->at(location/4))) >> 16);
				else if(location%2==0) halfLoaded = (unsigned short) ( (data->at(location/4)) & 0x0000ffff );
				data->getDataByVaddr(location, cycle);
				regs->at(rt) = (unsigned short)halfLoaded;
				break;

			case 0x20:	//lb 
				sumOverflow(aluValue1, aluValue2);
				if( location > 1023) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;

				byteLoaded = (signed char) ( ((unsigned int)(data->at(location/4))) >> ((location%4==0) ? 24 :
																						(location%4==1) ? 16 :
																						(location%4==2) ? 8  :
																						(location%4==3) ? 0 : 0) ) & 0x000000ff; 
				data->getDataByVaddr(location, cycle);
				regs->at(rt) = (signed char)byteLoaded;
				break;

			case 0x24:	//lbu
				sumOverflow(aluValue1, aluValue2);
				if( location > 1023 ) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;

				byteLoaded = (unsigned char) (((unsigned int)(data->at(location/4))) >> (	(location%4==0) ? 24 :
																							(location%4==1) ? 16 :
																							(location%4==2) ? 8  :
																							(location%4==3) ? 0 : 0) ) & 0x000000ff; 
				data->getDataByVaddr(location, cycle);
				regs->at(rt) = (unsigned char)byteLoaded;
				break;

			case 0x2B:	//sw
				sumOverflow(aluValue1, aluValue2);
				if ( location >1020 ) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if( location % 4 != 0 ){
					fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;
				data->at(location/4) = regs->at(rt);
				data->getDataByVaddr(location, cycle);
				break;

			case 0x29:	//sh
				sumOverflow(aluValue1, aluValue2);
				if ( location >1022 ) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if( location % 2 != 0 ){
					fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;
				
				data->at(location/4) &= (	(location%4==0) ? 0x0000ffff :
											(location%4==2) ? 0xffff0000 : 0xffff0000 );
				tempValue = regs->at(rt) & 0x0000ffff;
				tempValue <<= (	(location%4==0) ? 16 :
								(location%4==2) ? 0  : 0 );
				data->at(location/4) |= tempValue;
				data->getDataByVaddr(location, cycle);
				break;

			case 0x28:	//sb
				sumOverflow(aluValue1, aluValue2);
				if ( location >1023 ) {
					fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
					toReturn = -1;
				}
				if(toReturn!=0) return toReturn;
				
				data->at(location/4) &= (	(location%4==0) ? 0x00ffffff :
											(location%4==1) ? 0xff00ffff :
											(location%4==2) ? 0xffff00ff :
											(location%4==3) ? 0xffffff00 : 0xffffff00 );
				tempValue = regs->at(rt) & 0x000000ff;
				tempValue <<= (	(location%4==0) ? 24 :
								(location%4==1) ? 16 :
								(location%4==2) ? 8  :
								(location%4==3) ? 0  : 0 );
				data->at(location/4) |= tempValue;
				data->getDataByVaddr(location, cycle);
				break;

			case 0x0F:	//lui 
				if(toReturn!=0) return toReturn;
				regs->at(rt) = ((unsigned int)immediate) << 16;
				break;

			case 0x0C:	//andi 
				if(toReturn!=0) return toReturn;
				regs->at(rt) = regs->at(rs) & ( (unsigned short)immediate );
				break;

			case 0x0D:	//ori 
				if(toReturn!=0) return toReturn;
				regs->at(rt) = regs->at(rs) | ( (unsigned short)immediate );
				break;

			case 0x0E:	//nori 
				if(toReturn!=0) return toReturn;
				regs->at(rt) = ~( regs->at(rs) | ( (unsigned short)immediate ) );
				break;
			
			case 0x0A:	//slti
				if(toReturn!=0) return toReturn;
				if( ((signed int)regs->at(rs)) < (signed short)immediate ) regs->at(rt) = 1;
				else regs->at(rt) = 0;
				break;

			case 0x04:	//beq
				if(toReturn!=0) return toReturn;
				if( regs->at(rs)==regs->at(rt) ) PC += (4*(signed short)immediate);
				break;

			case 0x05:	//bne 
				if(toReturn!=0) return toReturn;
				if( regs->at(rs)!=regs->at(rt) ) PC += (4*(signed short)immediate);
				break;
			
			case 0x07:	//bgtz 
				if(toReturn!=0) return toReturn;
				if ((signed int)regs->at(rs) > 0) PC += (4*(signed short)immediate);
				break;
			//--------------------------- I type end -----------------------------//


			//--------------------------- J type start -----------------------------//
			case 0x02:	//j
				if(toReturn!=0) return toReturn;
				PC &= 0xf0000000;
				PC |= ( ((unsigned int)address) << 2 );
				break;

			case 0x03:	//jal
				if(toReturn!=0) return toReturn;
				regs->at(31) = PC;
				PC &= 0xf0000000;
				PC |= ( ((unsigned int)address) << 2 );
				break;
			//--------------------------- J type end -----------------------------//


			
			case 0x3f:	// halt
				return -1;
				break;
			default:
				fputs("no such instruction", stderr);
				break;
		}
	}

	return 0;
}



void print_snapshot(void)
{
	fprintf(snapshot , "cycle %d\n", cycle);
	for(unsigned int i=0 ; i<regs->size() ; i++){
		fprintf(snapshot, "$%02d: 0x%08X\n", i, (unsigned int)regs->at(i));
	}
	fprintf(snapshot, "PC: 0x%08X\n", PC);
	fprintf(snapshot, "\n\n");
}




void destroy_all(void)
{
	delete data;
	delete instructions;
	delete regs;

	fclose(snapshot);
	fclose(error_dump);
}



void sumOverflow(int aluValue1, int aluValue2)
{
	if(aluValue1<=0 && aluValue2<=0 && aluValue1+aluValue2 > 0){
		fprintf(error_dump, "In cycle %d: Number Overflow\n", cycle);
	}
	if(aluValue1>=0 && aluValue2>=0 && aluValue1+aluValue2 < 0){
		fprintf(error_dump, "In cycle %d: Number Overflow\n", cycle);
	}
	if(aluValue1<0 && aluValue2<0 && aluValue1+aluValue2==0){
		fprintf(error_dump, "In cycle %d: Number Overflow\n", cycle);
	}
}


void print_report()
{
	FILE *report = fopen("report.rpt", "w");
	fprintf( report, "ICache :\n");
	fprintf( report, "# hits: %u\n", instructions->cache_hit );
	fprintf( report, "# misses: %u\n\n", instructions->cache_miss );
	
	fprintf( report, "DCache :\n");
	fprintf( report, "# hits: %u\n", data->cache_hit );
	fprintf( report, "# misses: %u\n\n", data->cache_miss );
	
	fprintf( report, "ITLB :\n");
	fprintf( report, "# hits: %u\n", instructions->TLB_hit );
	fprintf( report, "# misses: %u\n\n", instructions->TLB_miss );
	
	fprintf( report, "DTLB :\n");
	fprintf( report, "# hits: %u\n", data->TLB_hit );
	fprintf( report, "# misses: %u\n\n", data->TLB_miss );
	
	fprintf( report, "IPageTable :\n");
	fprintf( report, "# hits: %u\n", instructions->pageTable_hit );
	fprintf( report, "# misses: %u\n\n", instructions->pageTable_miss );
	
	fprintf( report, "DPageTable :\n");
	fprintf( report, "# hits: %u\n", data->pageTable_hit );
	fprintf( report, "# misses: %u\n\n", data->pageTable_miss );
	
	fclose(report);
}
/*
std::vector<unsigned int>* readImage(FILE *image)
{
	std::vector<unsigned int> *input = new std::vector<unsigned int>;
	input->reserve(256);
	unsigned char readByte;
	unsigned int count=0, temp=0;
	while(fread(&readByte, sizeof(unsigned char), 1, image) != 0){
		temp <<= 8;
		temp |= readByte;
		count++;
		if(count >= 4){
			input->push_back(temp);
			temp = count = 0;	
		}
	}
	//printf("input has %lu words\n", input->size()) ;
	return input;
}
*/