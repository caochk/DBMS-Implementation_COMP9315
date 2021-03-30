// bits.c ... functions on bit-strings
// part of signature indexed files
// Bit-strings are arbitrarily long byte arrays
// Least significant bits (LSB) are in array[0]
// Most significant bits (MSB) are in array[nbytes-1]

// Written by John Shepherd, March 2019

#include <assert.h>
#include "defs.h"
#include "bits.h"
#include "page.h"

typedef struct _BitsRep {
	Count  nbits;		  // how many bits
	Count  nbytes;		  // how many bytes in array
	Byte   bitstring[1];  // array of bytes to hold bits
	                      // actual array size is nbytes
} BitsRep;

// create a new Bits object

Bits newBits(int nbits)
{
	Count nbytes = iceil(nbits,8); // count is actually unsigned int/iceil函数本质就是上取整函数
	Bits new = malloc(2*sizeof(Count) + nbytes); // BitsRep*结构体类型创建变量new, 并为其分配空间
	new->nbits = nbits; // 结构体赋值
	new->nbytes = nbytes; //结构体赋值
	memset(&(new->bitstring[0]), 0, nbytes);//该函数用于给前nbytes位赋0
	return new;
}

// release memory associated with a Bits object

void freeBits(Bits b)
{
	//TODO
	free(b);
}

// check if the bit at position is 1

Bool bitIsSet(Bits b, int position)
{
	assert(b != NULL);//如果b为null，系统报错并中断
	assert(0 <= position && position < b->nbits);//如果position<0或者传入的position比nbits大了，系统报错中断
	//TODO
	int positionInArray = position / 8;
	int positionInElement = position % 8;
    return b->bitstring[positionInArray] & (1 << positionInElement);
}

// check whether one Bits b1 is a subset of Bits b2

Bool isSubset(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
    for (int i = 0; i < b1->nbytes; ++i) {
        if ((b1->bitstring[i] & b2->bitstring[i]) != b1->bitstring[i]) {
            return FALSE;
        }
    }
	return TRUE;
}

// set the bit at position to 1

void setBit(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO
    int positionInArray = position / 8;
    int positionInElement = position % 8;
    b->bitstring[positionInArray] = b->bitstring[positionInArray] | (1 << positionInElement);
}

// set all bits to 1

void setAllBits(Bits b)
{
	assert(b != NULL);
	//TODO
    memset(&(b->bitstring[0]), 1, b->nbytes); //不确定有没有问题
}

// set the bit at position to 0

void unsetBit(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO
    int positionInArray = position / 8;
    int positionInElement = position % 8;
    b->bitstring[positionInArray] = b->bitstring[positionInArray] & ~(1 << positionInElement);
}

// set all bits to 0

void unsetAllBits(Bits b)
{
	assert(b != NULL);
	//TODO
    memset(&(b->bitstring[0]), 0, b->nbytes);
}

// bitwise AND ... b1 = b1 & b2

void andBits(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
    for (int i = 0; i < b1->nbytes; ++i) {
        b1->bitstring[i] = (b1->bitstring[i] & b2->bitstring[i]);
    }
}

// bitwise OR ... b1 = b1 | b2

void orBits(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
    for (int i = 0; i < b1->nbytes; ++i) {
        b1->bitstring[i] = (b1->bitstring[i] | b2->bitstring[i]);
    }
}

// left-shift ... b1 = b1 << n
// negative n gives right shift

void shiftBits(Bits b, int n) //不确定对不对
{
    // TODO
    assert(n < 8 || n > -8);
    if (n >= 0) {
        for (int i = 0; i < b->nbytes; ++i) {
            b->bitstring[i] << n;
        }
    }
    else {
        for (int i = 0; i < b->nbytes; ++i) {
            b->bitstring[i] >> n;
        }
    }

}

// get a bit-string (of length b->nbytes)
// from specified position in Page buffer
// and place it in a BitsRep structure

void getBits(Page p, Offset pos, Bits b)
{
	//TODO
    assert(p != NULL && b != NULL);
    Byte *addr = addrInPage(p, pos, b->nbytes);
    memcpy(b->bitstring, addr, b->nbytes);
}

// copy the bit-string array in a BitsRep
// structure to specified position in Page buffer

void putBits(Page p, Offset pos, Bits b)
{
	//TODO
    assert(p != NULL && b != NULL);
    Byte *addr = addrInPage(p, pos, b->nbytes);
    memcpy(addr, b->bitstring, b->nbytes);
}

// show Bits on stdout
// display in order MSB to LSB
// do not append '\n'

void showBits(Bits b)
{
	assert(b != NULL);
    //printf("(%d,%d)",b->nbits,b->nbytes);
	for (int i = b->nbytes-1; i >= 0; i--) {
		for (int j = 7; j >= 0; j--) {
			Byte mask = (1 << j);
			if (b->bitstring[i] & mask)
				putchar('1');
			else
				putchar('0');
		}
	}
}
