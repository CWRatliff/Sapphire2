// 210707 - Find() - reversed order of KeyCompare
//			so test key is key terminator
// 210709 - Next & Prev return 1 if new key != starting key
// 210710 - added GetKeyBody to Next & Prev to avoid comparing recno's
//		  - added ndxtype (Pri or Sec) to Next & Prev to distinguish
//			existence of trailing recno

#include "pch.h"
#include "OS.h"
#include <string.h>
#include <stdio.h>
#include "dbdef.h"
#include "RField.h"
#include "RData.hpp"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RPage.hpp"
#include "RNode.hpp"
#include "RBtree.hpp"
#include "RIndex.h"
#include "utility.h"
#include "str_ing.h"

// Root/Branch record layout
// +-------+----+----+---+----+-+----+
// | ndxno | K1 | K2 |...| Kn |0| Pi |   K1 can be a recno for data indexes
// +-------+----+----+---+----+-+----+
// |<--------- key area ------->|

// Primary index with data record layout in leaf form
// +-------+-------+-+----+----+-----+-+
// | ndxno | recno |0| f0 | f1 | ... |0|
// +-------+-------+-+----+----+-----+-+
// |<--- key area -->|<-- data area -->|

// Secondary index record layout in leaf form
// +-------+----+----+----+-------+-+-+
// | ndxno | k0 | k1 |... | recno |0|0|
// +-------+----+----+----+-------+-+-+
// |<---------- key area ---------->|^|
//                                    \
//                                     data area
// ndxno layout
// +-----+---------+
// | INT |  ndx #  |
// +-----+---------+
// |<sizeof(int)+1>|

// recno layout
// +-----+---------+
// | INT |  rec #  |
// +-----+---------+
// |<sizeof(int)+1>|


// Notes:
//	1. ndxFld[0] is the ndxno, not the first user supplied key field
//	2. Consequently, the number of fields ndxFldCnt is 1 greater than the 
//		number of user supplied key fields
//	3. Search keys may or may not include the recno tail field
//	4. Search keys MUST have at least the ndxno field
//  5. All node pointers i.e. P0, P1...Pn, Left Sib & Right Sib 
//		are 4 byte integers without descriptor byte or terminator byte
//  6. ndxno and recno are 4 byte integers with descriptor byte

RIndex::~RIndex() {
	RField*	fld;
	delete [] ndxName;
	fld = ndxFld[0];			// all other RFields belong to other objects
	if (fld)
		delete fld;
	}
//==============================================================
// Start new index
RIndex::RIndex(const char* ndxname, int ndxno, RBtree *ndxbtree) {
	int	len = str_len(ndxname) + 1;
	ndxName = new char[len];
	str_cpy(ndxName,ndxname);

	ndxNo = ndxno;
	ndxBTree = ndxbtree;
	// zeroth key item is ndxNo, RIndex is data area "owner"
	ndxFld[0] = new RField("ndxNo", (char*)&ndxNo, INT, sizeof(int));
	ndxType[0] = INT;
	ndxFldCnt = 0;
	ndxLink = NULL;
	ndx_ID.ndxStatus = UNPOSITIONED;
	}
//==============================================================
// Add a new field to the index field list
// Used during MakeRelation, MakeIndex, and OpenRelation
// could cause chaos if used elsewhere
int RIndex::AddField(RField& fld, int desc) {
	ndxFldCnt++;
	ndxFld[ndxFldCnt] = &fld;
	ndxType[ndxFldCnt] = fld.GetType() | desc;
	return (0);
	}
//==============================================================
// Add a new secondary index record
int	RIndex::Add(int recno) {
	RKey	key;
	RKey	rkey;
	RData	data("\0");
	char	keystr[KEYMAX];

	ItemBuild(keystr, ndxFldCnt + 1, ndxFld, ndxType);
	key.SetKey(keystr);
	rkey.MakeSearchKey("i", recno);		// add data recno to key
	key.KeyAppend(rkey);
	ndxBTree->Add(&ndx_ID, key, data);
	return (0);
	}
//==============================================================
// delete 'this' index record
int RIndex::Delete() {
	int		rc;

	// 240601 - TBD - how about Find
	rc = ndxBTree->Search(&ndx_ID, ndx_ID.ndxKey);
	if (rc == 0)					// expect a direct hit
		rc = ndxBTree->Delete(&ndx_ID);
	return (rc);
	}
//==============================================================
// access the data record associated with current position
// return the data into a RField list
int	RIndex::Fetch(RField* fldlst[]) {
	RField*	fld;
	const char*	item;

	item = ndxBTree->GetData(&ndx_ID);		// skip over key part
	ItemDistribute(item, fldlst);			// distribute data fields
	for (int i = 0;; i++) {
		fld = fldlst[i];
		if (fld == NULL)
			break;
		fld->ClearFieldStatus();			// mark field status as UNCHANGED
		}
	return (0);
	}
//==============================================================
// access specified data record
// return the data into a RField list
int	RIndex::Fetch(int recno, RField* fldlst[]) {
	RField*	fld;
	RKey	key;
	const char*	item;
	int		rc;

	key.MakeSearchKey("ii", ndxNo, recno);
	rc = ndxBTree->Search(&ndx_ID, key);
	if (rc != 0)							// expect a direct hit
		return -1;							// we went past the end of this keyno
	item = ndxBTree->GetData(&ndx_ID);		// skip over key part
	ItemDistribute(item, fldlst);			// distribute data fields
	for (int i = 0;; i++) {
		fld = fldlst[i];
		if (fld == NULL)
			break;
		fld->ClearFieldStatus();			// mark field status as UNCHANGED
		}
	return (0);
	}
//==============================================================
// Search for a key (or partial key)
// rc = -1: error
// rc = 0: exact match (might have fewer components)
// rc = 1: suppled key smaller than found key
/*  240528 - noticed code is not used
int RIndex::DbSearch(const char* key) {
	RKey	ndxkey;
	RKey	skey;
	int		rc;

	skey.SetKey(key);
	ndxkey.MakeSearchKey("i", ndxNo);		// prepend ndxNo to caller's key
	ndxkey.KeyAppend(skey);
	rc = ndxBTree->Search(&ndx_ID, ndxkey);
	if (ndx_ID.ndxKey.GetKeyHead() != ndxNo)
		return -1;							// we went past the end of this keyno
	rc = ndxkey.KeyCompare(ndx_ID.ndxKey);
	// reassign return code
	if (rc == -2)							// if equal but key smaller than index key
		rc = 0;
	if (rc == -1)							// key smaller is OK, remap
		rc = 1;
	return (rc);
}
*/
//==============================================================
// Search for a key (or partial key)
// rc = -1: error
// rc = 0: exact match (might have fewer components)
// rc = 1: suppled key smaller than found key
int RIndex::Find(RKey &key) {
	RKey	ndxkey;
	int		rc;

	ndxkey.MakeSearchKey("i", ndxNo);		// prepend ndxNo to caller's key
	ndxkey.KeyAppend(key);
	rc = ndxBTree->Search(&ndx_ID, ndxkey);
	if (ndx_ID.ndxKey.GetKeyNdxNo() != ndxNo)
		return -1;							// we went past the end of this keyno
	rc = ndx_ID.ndxKey.KeyCompare(ndxkey);	// key order rqd. for search key terminator
	// reassign return code
	if (rc == -2)						// exact match but fewer fields
		rc = 0;							// if equal but key smaller than index key
	return (rc);
	}
//==============================================================
// position of smallest key in an index set
// rc of 0 expected, BETWEEN, since recno not part of search key
int RIndex::First() {
	RKey	ndxkey;
	int		rc;

	ndxkey.MakeSearchKey("i", ndxNo);		// prepend ndxNo to caller's key
	rc = ndxBTree->Search(&ndx_ID, ndxkey);
	if (rc < 0)								// neither ONKEY nor BETWEEN
		return -1;
	if (ndx_ID.ndxKey.GetKeyNdxNo() != ndxNo)
		return -1;						// we went past the end of this keyno
	return (rc);
	}
//==============================================================
// return the current key (without the prepended ndxno)
int RIndex::GetCurrentKey(RKey* key) {
	char*	keystr;

	keystr = ndx_ID.ndxKey.GetKeyStr();
	keystr += 5;							// skip over ndxno
	key->SetKey(keystr);
	return (0);
	}
//==============================================================
// scan linked list to find the ndxname associated with an ndx handle
RIndex* RIndex::GetIndexHandle(const char* ndxname) {
	RIndex*	ndx;

	for (ndx = this; ndx; ndx = ndx->ndxLink) {
		if (strcmp(ndx->ndxName, ndxname) == 0)
			return ndx;
		}
	return (0);
	}
//==============================================================
// return the recno of the last item in an index
int RIndex::GetLastRecNo() {
	RKey	key;
	int		recno;

	Last();									// go last item in this index
	if (ndx_ID.ndxKeyNo < 0)
		return (0);
	if (ndxNo != ndx_ID.ndxKey.GetKeyNdxNo()) // Prev failed (maybe no records)
		return 0;
	recno = GetRecno();
	return (recno);
	}
//==============================================================
// return the record number from a data record (2nd key component)
int RIndex::GetRecno() {
	int	recno;
	recno = ndx_ID.ndxKey.GetKeyRecNo();
	return (recno);
	}
//==============================================================
// position to "first" in next larger ndxNo and then backup one item
int RIndex::Last() {
	RKey	ndxkey;
	int		rc;

	ndxkey.MakeSearchKey("i", ndxNo+1);	// prepend ndxNo to caller's key
	rc = ndxBTree->Search(&ndx_ID, ndxkey);
	if (rc >= 0) {
		rc = ndxBTree->MovePrevious(&ndx_ID);
		if (ndx_ID.ndxKey.GetKeyNdxNo() != ndxNo)
			return -1;						// no items in ndxNo
		}
	return (rc);
	}
//==============================================================
// advance index one record
// return value -1 indicates index was not prepositioned or MoveNext()
// went beyond end-of-index
//  -1 -> past EOI
//   0 -> still on matching key
//   1 -> key does not match starting search key
int RIndex::Next(int ndxtype) {
	RKey	prekey;
	RKey	postkey;
	int		res;

	if (ndxtype == 0)			// primary (data) index
		prekey = ndx_ID.ndxKey;
	else						// 2ndary index
		prekey.GetKeyBody(ndx_ID.ndxKey);
	printf("Next prekey");
	prekey.PrintKey();
	printf("\n");
	if (ndx_ID.ndxStatus == IEOF || ndx_ID.ndxStatus == UNPOSITIONED)
		return (-1);
	int rc = ndxBTree->MoveNext(&ndx_ID);
	if (rc < 0)
		return (-1);
	if (ndx_ID.ndxKey.GetKeyNdxNo() != ndxNo)
		return -1;				// we went past the end of this keyno
	if (ndxtype == 0)
		res = prekey.KeyCompare(ndx_ID.ndxKey);
	else {
		postkey.GetKeyBody(ndx_ID.ndxKey);
		res = prekey.KeyCompare(postkey);
		}
	if (res < 0)				// if ikey < tkey
		return (1);
	return (0);
	}
//==============================================================
// back up one record
// return value -1 indicates index was not prepositioned or MoveNext() 
// went beyond end-of-index
int RIndex::Prev(int ndxtype) {
	RKey	prekey;
	RKey	postkey;
	int		res;

	if (ndxtype == 0)			// primary (data) index
		prekey = ndx_ID.ndxKey;
	else
		prekey.GetKeyBody(ndx_ID.ndxKey); // 2ndary index
	if (ndx_ID.ndxStatus == IEOF || ndx_ID.ndxStatus == UNPOSITIONED)
		return (-1);
	int rc = ndxBTree->MovePrevious(&ndx_ID);
	if (rc < 0)
		return (-1);
	if (ndx_ID.ndxKey.GetKeyNdxNo() != ndxNo)
		return -1;				// we went before the start of this keyno
	if (ndxtype == 0)
		res = prekey.KeyCompare(ndx_ID.ndxKey);
	else {
		postkey.GetKeyBody(ndx_ID.ndxKey);
		res = prekey.KeyCompare(postkey);
	}
	if (res < 0)
		return (1);
	return (0);
	}
//==============================================================
// build a keystring for every index in linked list
// N.B. assumes fields already filled
// assumes 'this' is Root index
int	RIndex::PresetIndexes(int recno) {
	RIndex* ndx;
	RKey	key;
	char	keystr[KEYMAX];

	key.MakeSearchKey("i", recno);
	for (ndx = this; ndx; ndx = ndx->ndxLink) {
		ItemBuild(keystr, ndx->ndxFldCnt+1, ndx->ndxFld);
		ndx->ndx_ID.ndxKey.SetKey(keystr);	// base key (with ndxno)
		ndx->ndx_ID.ndxKey.KeyAppend(key);	// append recno
		}
	return (0);
	}
//==============================================================
// if any fields of 'this' index have changed, search for the 
// old index on the btree delete it and add a new index onto the btree
int	RIndex::UpdateIndex() {
	int		recno;
	int		i;
	int		update = 0;

	for (i = 1; i < (ndxFldCnt + 1); i++) {	// see if any fields were changed
		if (ndxFld[i]->GetFieldStatus() == TRUE)
			update++;
		}
	if (update == 0)					// no changes, leave me alone
		return 0;

	ndxBTree->Search(&ndx_ID, ndx_ID.ndxKey);	// expect a direct hit
	if (ndx_ID.ndxKey.GetKeyNdxNo() != ndxNo)
		return -1;					// we went past the end of this keyno
	recno = GetRecno();
	Delete();
	Add(recno);
	return 0;
	}
//==============================================================
int	RIndex::SetLink(RIndex* ndx) {
	ndxLink = ndx;
	return (0);
	}
//==============================================================
// write a data item with ndxno:recno key
int RIndex::WriteData(RData &data, int recno) {
	RKey	datakey;
	int		rc;
	
	datakey.MakeSearchKey("ii", ndxNo, recno);
	rc = ndxBTree->Add(&ndx_ID, datakey, data);
	return (rc);
	}
//==============================================================
void RIndex::PrintIndex() {
	printf("Index: %s, ndxNo %d, Cnt %d, status %d\n", ndxName, ndxNo, ndxFldCnt, ndx_ID.ndxStatus);
	printf("   Node# %d, keyno# %d", ndx_ID.ndxNode, ndx_ID.ndxKeyNo);
	ndx_ID.ndxKey.PrintKey();
	printf("\n");
	}
