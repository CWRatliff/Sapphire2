#include "pch.h"
#include "OS.h"
#include <string.h>
#include "RField.h"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "dbdef.h"
#include "utility.h"
#include "str_ing.h"

//=============================================================================
RField::RField() {
	fldName = NULL;
	fldData = NULL;
	fldType = 0;
	fldLen = 0;
	fldOwner = FALSE;		// data area owned elsewhere
	fldChg = 0;
	}
//=============================================================================
RField::RField(const char* fldname, char* flddata, int fldtype, int fldlen) {
	int	len = str_len(fldname) + 1;
	fldName = new char[len];
	str_cpy(fldName, fldname);
	fldData = flddata;
	fldType = fldtype;
	fldLen = fldlen;
	fldChg = FALSE;
	fldOwner = FALSE;		// data area owned elsewhere
	}
//=============================================================================
RField::RField(const char* fldname, int fldtype, int fldlen) {
	int	len = str_len(fldname) + 1;
	fldName = new char[len];
	str_cpy(fldName, fldname);
	fldType = fldtype;
	if (fldtype == INT)
		fldLen = sizeof(int);
	else if (fldtype == FP)
		fldLen = sizeof(float);
	else if (fldtype == DP)
		fldLen = sizeof(double);
	else
		fldLen = fldlen + 1;
	fldData = new char[fldLen+1];
	fldChg = 0;
	fldOwner = TRUE;		// data area owned by 'this'
	}
//=============================================================================
RField::RField(const char* fldname, int fldtype) {
	int	len = str_len(fldname) + 1;
	fldName = new char[len];
	str_cpy(fldName, fldname);
	fldType = fldtype;
	if (fldtype == INT)
		fldLen = sizeof(int);
	else if (fldtype == FP)
		fldLen = sizeof(float);
	else if (fldtype == DP)
		fldLen = sizeof(double);
	else
		fldLen = FLDMAX;
	fldData = new char[fldLen+1];
	fldChg = 0;
	fldOwner = TRUE;		// data area owned by 'this'
	}
//=============================================================================
RField::~RField() {
	if (fldName)
		delete [] fldName;
	if (fldOwner && fldData)
		delete [] fldData;
	}
//=============================================================================
int RField::ClearField() {
	double	zero = 0;
	
	if (fldType & STRING)
		str_cpy(fldData, "");				// empty string
	else if (fldType == FP)
		memcpy(fldData, &zero, sizeof(float));	// zero
	else if (fldType == DP)
		memcpy(fldData, &zero, sizeof(double));	// zero
	else
		memcpy(fldData, &zero, sizeof(int));	// zero
	fldChg = TRUE;
	return (0);
	}
//=============================================================================
// fetch a char pointer to character type field data
const char* RField::GetCharPtr() {
	const char* p;

	p = fldData;
	return (p);
	}
//===========================================================================
int	RField::GetCharCopy(char* data, int len) {
	char*	p;
	int		flen;

	p = fldData;
	flen = fldLen;
	if (len > flen)
		len = flen;
	str_ncpy(p, data, len);
	data[len - 1] = '\0';
	return len;
	}
//=============================================================================
// fetch an int field
int RField::GetInt() {
	int		i = 0;

	memcpy(&i, fldData, sizeof(int));
	return (i);
	}
//=============================================================================
// fetch a float field
float RField::GetFloat() {
	float		f = 0;

	memcpy(&f, fldData, sizeof(float));
	return (f);
	}
//=============================================================================
// fetch a double field
double RField::GetDouble() {
	double		d = 0;

	memcpy(&d, fldData, sizeof(double));
	return (d);
	}
//=============================================================================
int RField::SetData(const char *data) {
	int	len;

	str_ncpy(fldData, data, fldLen);
	fldData[fldLen] = '\0';
	len = str_len(data) + 1;
	fldChg = TRUE;
	return (len);
	}
//=============================================================================
int RField::SetData(int data) {

	memcpy(fldData, &data, sizeof(int));
	fldChg = TRUE;
	return (sizeof(int));
	}
//=============================================================================
int RField::SetData(float data) {

	memcpy(fldData, &data, sizeof(float));
	fldChg = TRUE;
	return (sizeof(float));
	}
//=============================================================================
int RField::SetData(double data) {

	memcpy(fldData, &data, sizeof(double));
	fldChg = TRUE;
	return (sizeof(double));
	}

