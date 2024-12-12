#include "pch.h"
#include "OS.h"
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include "RField.h"
#include "RKey.hpp"
#include "RData.hpp"
#include "Ndbdefs.h"
#include "utility.h"
#include "dbdef.h"
#include "str_ing.h"
//    datStr
//    |
//    v
//   +----+------+----+------+-----------+---+
//   | t1 | d1   | t2 | d2   | ...       | 0 | 
//   +----+------+----+------+-----------+---+f
//   <--------------- datLen ---------------->

// Data types
// 		1xxx xxxx	- descending order
//		x010 0000	- alphanumeric string (zero terminated)
//		x110 0000	- alpha string uncased
//		x000 0100	- numeric string
//		x000 0001	- integer (32-bit)
//		x000 0010	- floating point
//		x000 0011	- double precision
//		x000 0101	- long long integer (64-bit)

//		0000 0000	- terminator

//==================================================================
RData::RData() {
	datStr = new char[DATAMAX];
	datLen = 0;
	}
// memcpy is used for int - no guarantee of word alignment
//==================================================================
/*
	create an RData object using "item"
	item is a mixed string of datatypes and data
*/
RData::RData(char* item) {
	datStr = new char[DATAMAX];
	datLen = SetData(item);
	}

// memcpy is used for int, long, et. al. - no guarantee of word alignment
//==================================================================
RData::RData(char *item, int dlen) {
/*
	create an RData object using a given data stream "item"
	whose total length is known
*/
	datStr = new char[DATAMAX];
	memcpy(datStr, item, dlen);
	datLen = dlen;
	}
//==================================================================
RData::RData(const char *item) {
	/*
	create an RData object using a given data stream "item"
	*/
	datStr = new char[DATAMAX];
	str_cpy(datStr, item);
	datLen = str_len(item) + 1;
}
//==================================================================
RData::~RData() {
	if (datStr)
		delete [] datStr;
	}
//==================================================================
// determine length of a formatted data string (GetDataLen)
int RData::SetData(char* str) {
	char	*p;
	int		idb;
	int		ilen;
	int		itemlen = 0;

	p = str;
	while (*p) {
		itemlen++;
		idb = *p++;
		if (idb & STRING) {
			ilen = str_len(p) + 1;
			itemlen += ilen;
			p += ilen;
			}
		else if (idb == FP) {
			ilen = sizeof(float);
			itemlen += ilen;
			p += ilen;
			}
		else if (idb == DP) {
			ilen = sizeof(double);
			itemlen += ilen;
			p += ilen;
			}
		else if (idb == INT) {
			ilen = sizeof(int);
			itemlen += ilen;
			p += ilen;
			}
		}
	datLen = itemlen + 1;
	memmove(datStr, str, datLen);
	p = datStr + itemlen;
	*p = '\0';				// terminator

	return (datLen);
	}
//===================================================================
void RData::PrintData() {

	if (datStr[0] != 0) {
		printf("Data: ");
		ItemPrint(datStr);
		}
	}
