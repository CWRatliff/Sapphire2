// Keys are cardinal at Node level (0 relative at page level)


#include "pch.h"
#include "OS.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "RField.h"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RPage.hpp"
#include "RData.hpp"
#include "RNode.hpp"

#ifdef MSDOS
#include <io.h>
extern	errno_t	err;
#endif
#ifdef LINUX
#include <sys/types.h>
#include <unistd.h>
//extern	int	err;
#endif

/* Symbolic index node format
	+----+----+----+----+----+----+----+-------+----+----+
	| P0 | LS | RS | K1 | P1 | K2 | P2 |  ...  | Kn | Pn |
 	+----+----+----+----+----+----+----+-------+----+----+   */

// Ki:Pi are composites - Ki consists of a Key part Pi is a node #
// LS - Left Sibling node #
// RS - Right Sibling node #
// P0 -> items < K1:P1
// so that n keys will have n+1 pointers to other nodes
// node # is a positive integer corresponding to a nodesize space on disk
// node zero has space available info, the root node is always node # 1
// K1 < K2 < ... < Kn    allowing binary search

// di data items exists only on leaf level nodes
// Note that nodes treat keys as cardinals (i.e. 1 to n), pages are zero based
// leaf nodes have no Pi items
/*
	+----+----+----+-------+-------+-------+-------+
	| 0  | LS | RS | k1:d1 | k2:d2 |  ...  | kn:dn |
 	+----+----+----+-------+-------+-------+-------+   */

// k1 < k2 < ... < kn    allowing binary search
// K1 == kn for some index node except for rightmost leaf node
// since RNode owns the nodeMemory it is allowed to read from it directly,
// even though it is managed exclusively by RPage
// almost a class friend situation
//==================================================================
//RNode::RNode(const int fileHandle, const NODE fileAvail) {
//	nodeFd = fileHandle;
//	nodeCurr = 0;
//	nodeAvail = fileAvail;
//	Clear();
//}
//==================================================================
RNode::RNode(const int fileHandle) {
	nodeFd = fileHandle;
	nodeCurr = 0;
	Clear();
}
//==================================================================
// See if node has enough empty space to hold a specified key
int RNode::IsBigEnough(const int len) {
	return (nodePage.IsBigEnough(len));
	}	
//==================================================================
// Clear a node back to initial conditions
void RNode::Clear() {
	nodeLeftSibling = 0;
	nodeRightSibling = 0;
	nodeP0 = 0;
	memset(&nodeMemory[0], 0, KEYSPACE);
	nodePage.Initialize(&nodeMemory[0], KEYSPACE);
	}
//==================================================================
// delete a key
int RNode::DeleteKey(const int keyNo) {
	nodePage.Delete(keyNo-1);				// just delete it
	return 0;
	}
//==================================================================
// delete an index key

/*	      +----+----+----+----+------+----+----+
	  P0  | K1 | P1 | K2 | P2 | ...  | Kn | Pn |
          +----+----+----+----+------+----+----+
*/

int RNode::DeleteIndex(const int keyNo) {
	RKey		key;
	NODE		nnNode;
	int			len;
	const char*	item;

/*	case 1: delete lowest node: K1
	
	      +----+----+------+----+----+
	  P1  | K2 | P2 | ...  | Kn | Pn |
 	      +----+----+------+----+----+
*/

	if (keyNo == 0) {							// if P0 is deleted, 
		nodeP0 = GetPi(1);						// grab P1 for new P0
		DeleteKey(1);							// delete K1,P1
		}
/*
	case 2: delete intermediate node ie: K2
	
	      +----+----+------+----+----+
	  P0  | K1 | P2 | ...  | Kn | Pn |
          +----+----+------+----+----+	
*/

	else if (keyNo < nodePage.GetSlots()) {
		nnNode = GetPi(keyNo+1);				// extract ptr
		item = nodePage.GetDataItem(keyNo - 1);
		key.SetKey(item);
		len = key.GetKeyLen();
		DeleteKey(keyNo+1);						// remove key
		nodePage.Insert((char *)&nnNode, LNODE, keyNo-1, len);
		}
/*	 case 3: delete RHE node

	      +----+----+----+----+------+------+------+
	  P0  | K1 | P1 | K2 | P2 | ...  | Kn-1 | Pn-1 |
 	      +----+----+----+----+------+------+------+
*/

	else										// last key
		DeleteKey(keyNo);

	return 0;
	}
//==================================================================
// delete a node from the file and recycle it's disk space
int RNode::DeleteNode() {
	RNode	work(nodeFd);
	NODE	avail;

	// revise left/right node links
	if (nodeLeftSibling) {
		work.ReadNode(nodeLeftSibling);
		work.nodeRightSibling = nodeRightSibling;
		work.WriteNode();
		}
	if (nodeRightSibling) {
		work.ReadNode(nodeRightSibling);
		work.nodeLeftSibling = nodeLeftSibling;
		work.WriteNode();
		}
	
	work.ReadNode(0);			// read NODE 0 to get avail NODE
	avail = work.nodeP0;	// now point to newly deleted NODE (nodeCurr)
	work.nodeP0 = nodeCurr;
	work.nodeLeftSibling = 0L;
	work.nodeRightSibling = 0L;
	work.WriteNode();

	nodeP0 = avail;			// deleted node now has avail link
	nodeLeftSibling = 0;
	nodeRightSibling = 0;
	WriteNode();

	return 0;
	}
//==================================================================
// returns a pointer to the data part of a key:data item
const char* RNode::GetData(const int keyno) {
	const char*	item;
	RKey	key;

	item = GetKi(keyno);
	GetKey(&key, keyno);		// boundry check?
	item = item + key.GetKeyLen(); // skip over key of key:data
	return (item);
	}
//==================================================================
// access a key from the node and make a Key object from it
//  entire key ndxno:recno for primary keys
//    ndxno:k0...:recno for secondary keys

void RNode::GetKey(RKey *key, const int keyno) {
	const char	*item;

	if (keyno > GetCount())
		item = GetKi(GetCount());
	else
		item = GetKi(keyno);
	key->SetKey(item);
	}
//==================================================================
// access RHE key from the node and make a Key object from it

void RNode::GetLastKey(RKey *key) {
	const char	*item;
	int		kn;

	kn = nodePage.GetSlots();
	item = GetKi(kn);
	key->SetKey(item);
	}
//==================================================================
// get the pointer part of an index key

NODE RNode::GetPi(const int i) {
	RKey	tkey;
	const char	*item;
	int		klen;
	NODE	Pi;

	if (i == 0)
		return nodeP0;
	item = GetItem(i);		// boundary check?
	tkey = item;					// make a key object
	klen = tkey.GetKeyLen();		// key length for skip over
	memcpy(&Pi, item+klen, LNODE);
	return (Pi);
	}
//==================================================================
// get the RHE index pointer
NODE RNode::GetPn() {
	int	kn;

	kn = nodePage.GetSlots();				// last item in page
	return (GetPi(kn));
	}
//==================================================================
// insert a key into the page (if there is room in the inn)
int RNode::InsertKey(const int keyNo, RKey &key) {
	int klen = key.GetKeyLen();
	if (nodePage.IsBigEnough(klen)) {
		nodePage.Allocate(keyNo-1, klen);
		nodePage.Insert(key.GetKeyStr(), klen, keyNo-1);
		return (1);
		}
	return (0);		
	}	
//==================================================================
// insert a key & data into the page (if there is room in the inn)
int RNode::InsertKeyData(const int keyNo, RKey &key, RData &data) {

	int klen = key.GetKeyLen();
	int dlen =  data.GetDataLen();

	if (nodePage.IsBigEnough(klen + dlen)) {
		nodePage.Allocate(keyNo-1, klen+dlen);
		nodePage.Insert(key.GetKeyStr(), klen, keyNo-1);
		if (dlen > 0)
			nodePage.Insert(data.GetDataStr(), dlen, keyNo-1, klen);
		return (1);
		}
	return (0);		
	}	
//==================================================================
// insert a key into the page (if there is room in the inn)
int RNode::InsertKeyPtr(const int keyNo, RKey &key, NODE node) {
	int klen = key.GetKeyLen();
	int len = klen + LNODE;
	if (nodePage.IsBigEnough(len)) {
		nodePage.Allocate(keyNo-1, len);
		nodePage.Insert(key.GetKeyStr(), klen, keyNo-1);
		nodePage.Insert((char *)&node, LNODE, keyNo-1, klen);
		return (1);
		}
	return (0);		
	}	
//==================================================================
// create a new node on disk
// N.B. data & siblings may exist in 'this' node, do not disturb
NODE RNode::NewNode() {
	int		rawpos;
	NODE	avail = 0L;
	RNode	work(nodeFd);

	work.ReadNode(0);			// read NODE 0 to get availf
	if (work.nodeP0 > 0) {
		nodeCurr = work.nodeP0;
		ReadNode(nodeCurr);
		work.nodeP0 = nodeP0;	// next link to node 0
		work.WriteNode();
		}
	else {
		rawpos = SeekEOF();
		assert(err == 0);
		nodeCurr = rawpos / NODESIZE;	// compute node number
		}

	//	printf("node %d created\n", nodeCurr);
	return(nodeCurr);
	}
//==================================================================
// Advance a key within the node
int RNode::NextKey(int keyno) {
	if (keyno < GetCount())
		return keyno + 1;
	return 0;
	}
//==================================================================
// Advance to right sibling
NODE RNode::NextNode() {
	NODE nnNext;

	nnNext = nodeRightSibling;
	if (nnNext) {
		ReadNode(nodeRightSibling);
		return nnNext;
		}
	return 0;
	}
//==================================================================
// Backup a key - not very useful
int RNode::PrevKey(int keyno) {
	return --keyno;
	}
//==================================================================
// Advance to left sibling
NODE RNode::PrevNode() {
	NODE nnPrev;

	nnPrev = nodeLeftSibling;
	if (nnPrev) {
		ReadNode(nodeLeftSibling);
		return nnPrev;
		}
	return 0;
	}
//==================================================================
// no RNode function should know about leafs, branches, or roots,
// just if Ptrs are used, and maybe not even that
void RNode::PrintNode() {
	RKey	key;
	RData	*data;
	int		recno;
	
	printf("Dump of node # %d", nodeCurr);
	if (nodeLeftSibling == 0 && nodeRightSibling == 0)
		printf(" (Root) ");
	else {
		if (nodeP0 == 0)
			printf(" (Leaf) ");
		else
			printf(" (Branch) ");
		printf(" L/R Sibs: %d / %d ", nodeLeftSibling, nodeRightSibling);
		}

	if (nodeP0 != 0)
		printf(" P0: %d ", nodeP0);
	printf("\n");;

	for (int i = 1; i <= nodePage.GetSlots(); i++) {
		printf(" %d ", i );
		GetKey(&key, i);
		key.PrintKey();
		if (nodeP0) {
			recno = GetPi(i);
			printf(" @%d", recno);
			}
		else {
			data = new RData((char *)GetData(i));
			data->PrintData();
			delete data;
			}
		printf("\n");
		}


		int i = nodePage.GetSlots();
		printf("Count: %d ", i );			// compact form

	printf("\n");
	}
//==================================================================
// scan a node for leftmost matching key using a binary search

// returns:
//		0 if key < K1
//		i if key == Ki
//		-i if Ki < key < Ki+1 or Kn < key

// note: for a right to left seek, set lo to i at if res == 0

int RNode::ScanNode(RKey &key) {
	int		lo, hi;
	int		i;
	int		res;
	const char*	ki;
	
	hi = GetCount();
	if (hi <= 0)
		return (0);					// empty node
	lo = 1;	
	while (lo <= hi) {
		i = (lo + hi) / 2;
		ki = GetKi(i);
		res = key.KeyCompare(ki);	// res = -1 < 0 < 1 less:equal:greater
		if (lo == hi) {				// end of scan
			if (res > 0)			// but key < Ki[hi]
				hi--;
			break;
			}
		if (res < 0)
			lo = i + 1;
		else if (res == 0)			// a hit becomes hi
			hi = i;					// there may be lower exact hits
		else
			hi = i - 1;				// hi eventually = 0 if key < K1
		}
	if (res == 0)
		return (hi);				// direct hit
	return (-hi);					// between Ki & Ki+1
 	} 
//==================================================================
// scan index node for an entry that points down to a descendant
int RNode::ScanNodeforP(NODE desc) {
	if (desc == GetPi(0))			// if leaf node
		return 0;

	for (int i = 0; i <= nodePage.GetSlots(); i++) {
		if (desc == GetPi(i))
			return i;
		}
	return -1;									// no find
	}
//==================================================================
// move right half of the keys of this node into node2
// revise sibling pointers

// I wonder if this function belongs in the node object. Maybe its function
// should be entirely in the tree object


/*	 +----+----+----+----+----+-----+----+----+----+----+----+----+----+
     | P0 | K1 | P1 | K2 | P2 | ... | kl | pl | kr | pr |... | Kn | Pn |
     +----+----+----+----+----+-----+----+----+----+----+----+----+----+
                                              ^
											  split


	 this											node2
	 +----+----+----+----+----+-----+----+----+		+----+----+----+----+----+
	 | P0 | K1 | P1 | K2 | P2 | ... | kl | pl |	<->	| kr | pr |... | Kn | Pn |<-
	 +----+----+----+----+----+-----+----+----+		+----+----+----+----+----+->
	                                             ^                            ^
												       revised siblings

*/   


int RNode::Split(RNode *node2) {
	RNode	work(nodeFd);
	NODE	nnRight;
	int		lKeys, rKeys;
	const char	*item;
	int		len, nlen;
	int		ttllen;
	int		half;
	
	// Split node into two nodes
	// split by memory size
	nlen = nodePage.GetSlots();
	ttllen = 0;
	half = nodePage.GetUsed() / 2;				// middle aiming point
	for (lKeys = 0; lKeys < nlen; lKeys++) {
		ttllen += nodePage.GetDataLen(lKeys);	// add up total size so far
		if (ttllen >= half)
			break;
		}
	rKeys = nlen - lKeys;

	// move right half of node into new node
	for (int i = 0; i < rKeys; i++) {
		item = nodePage.GetDataItem(lKeys+i);
		len = nodePage.GetDataLen(lKeys+i);
		node2->nodePage.Allocate(i, len);
		node2->nodePage.Insert(item, len, i);
		} 
	nodePage.Delete(lKeys, nlen-1);

	nnRight = GetRightSibling();			//	reconnect sibling pointers
	node2->SetRightSibling(nnRight);
	node2->SetLeftSibling(nodeCurr);
	SetRightSibling(node2->nodeCurr);
	if (nnRight) {							// if right exists, point to new node
		work.ReadNode(nnRight);
		work.SetLeftSibling(node2->nodeCurr);
		work.WriteNode();
		}

	printf("Split\nnew left node\n");
//	nodePage.Dump();
//	PrintNode();
//	printf("new right node\n");
//	node2->nodePage.Dump();
//	node2->PrintNode();
	return (lKeys);
	}
//==================================================================
// Read a node from disk into a node object
int RNode::ReadNode(NODE node) {
	int bytes;   

//	if (nodeCurr == node)
//		return 1;
#ifdef MSDOS
	_set_errno(0);
	_lseeki64(nodeFd, (long long)node*NODESIZE, SEEK_SET);
	_get_errno(&err);
//	assert(err == 0);
	bytes = _read(nodeFd, &nodeP0, NODESIZE);
	_get_errno(&err);
	assert(err == 0);
#endif
#ifdef LINUX
//	errno = 0;
	lseek(nodeFd, node*NODESIZE, SEEK_SET);
//	assert(errno == 0);
	bytes = readNode(nodeFd, &nodeP0, NODESIZE);
//	assert(errno == 0);
#endif

	if (bytes < NODESIZE)
		return 0;
//	nodePage.Reset(&nodeMemory[0], KEYSPACE);
	nodeCurr = node; 
	return 1;
	}
//================================================================
NODE RNode::SeekEOF() {
	NODE rawpos = 0;
#ifdef MSDOS
	_set_errno(0);
	rawpos = _lseeki64(nodeFd, 0, SEEK_END);		// EOF
	_get_errno(&err);
	assert(err == 0);
	_write(nodeFd, &nodeP0, NODESIZE);
	_get_errno(&err);
	assert(err == 0);
#endif
#ifdef LINUX
	rawpos = lseek(nodeFd, 0L, SEEK_END);		// EOF
	//	assert(errno == 0);
	write(nodeFd, &nodeP0, NODESIZE);
#endif
	return (rawpos);
	}
//==================================================================
// Write a node object onto disk
int RNode::WriteNode() {
#ifdef MSDOS
	_set_errno(0);
	_lseeki64(nodeFd, (long long)nodeCurr*NODESIZE, SEEK_SET);
	_get_errno(&err);
//	assert(err == 0);
	_write(nodeFd, &nodeP0, NODESIZE);
	_get_errno(&err);
//	assert(err == 0);
#endif
#ifdef LINUX
//	errno = 0;
	lseek(nodeFd, nodeCurr*NODESIZE, SEEK_SET);
//	assert(errno == 0);
	write(nodeFd, &nodeP0, NODESIZE);
//	assert(errno == 0);
#endif
	return 0;
	}
