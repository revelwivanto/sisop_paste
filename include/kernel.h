#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "std_type.h"

// extern int interrupt(int number, int AX, int BX, int CX, int DX);
extern int interrupt10(int AX, int BX, int CX, int DX);
extern int interrupt13(int AX, int BX, int CX, int DX);
extern int interrupt16(int AX, int BX, int CX, int DX);
extern int interrupt21(int AX, int BX, int CX, int DX);

void printString(char* str);
void readString(char* buf);
void clearScreen();

void readSector(byte* buf, int sector);
void writeSector(byte* buf, int sector);

#endif