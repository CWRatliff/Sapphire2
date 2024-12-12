// 180302 windows/dos errno put into create
// 180304 closer delete of dbfname to errno()
// 230919 LINUX 'err' eliminated

#include "pch.h"
#include "OS.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
//#include "dbdef.h"
#include "RField.h"
#include "RData.hpp"
#include "RKey.hpp"
#include "Ndbdefs.h"
#include "RPage.hpp"
#include "RNode.hpp"
#include "RBtree.hpp"
#include "RIndex.h"
#include "RTable.h"
#include "RDbf.h"
//#include "utility.h"
#include "str_ing.h"

#ifdef MSDOS
#include <io.h>
extern	errno_t	err;
#endif
#ifdef LINUX
#include <sys/types.h>
#include <unistd.h>

//extern	int	err;
#endif

//========================================================================
RDbf::RDbf(const char* dbfname, int dbuserno) {
	dbfFd = 0;
	int len = str_len(dbfname);
	dbfName = new char[len+1];
	str_cpy(dbfName, dbfname);
	dbfLink = NULL;
	dbfRelRoot = NULL;
	dbfBtree = NULL;
	dbUserNo = dbuserno;
	}
//========================================================================
RDbf::~RDbf() {
	RTable*	rel;
	RTable*	nxtrel;

	for (rel = dbfRelRoot; rel; rel = nxtrel) {
		nxtrel = rel->GetLink();
		delete rel;
		}
#ifdef MSDOS
	if (dbfFd > 0)
		_close(dbfFd);
#endif
#ifdef LINUX
	if (dbfFd > 0)
		close(dbfFd);
#endif
	delete [] dbfName;
	if (dbfBtree)
		delete dbfBtree;
	}
//======================================================================
// File oriented ops
//======================================================================
// return dos file number if AOK, else -1 (errno is set)
int	RDbf::Create(const char *dbfname) {
	char	zeros[NODESIZE];
	int		len;
	char*	dosname;

	len = str_len(dbfname);
	dosname = new char[len+6];
	str_cpy(dosname, dbfname);
	if (strstr(dosname, ".sdbf") == 0)
		str_cat(dosname, ".sdbf");
	memset(zeros, 0, NODESIZE);
#ifdef MSDOS
	_set_errno(0);
//   dbfFd = _open(dosname, _O_BINARY | _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE);
	_sopen_s(&dbfFd, dosname, _O_BINARY | _O_RDWR | _O_CREAT, _SH_DENYNO, _S_IREAD | _S_IWRITE);
	_get_errno(&err);
#endif
#ifdef LINUX
//	errno = 0;
	dbfFd = open(dosname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	//err = errno;
#endif

	delete [] dosname;
//	if (err > 0)
	if (dbfFd > 0) {

#ifdef MSDOS
		_write(dbfFd, zeros, NODESIZE);
#endif
#ifdef LINUX
		write(dbfFd, zeros, NODESIZE);
#endif
		dbfBtree = new RBtree(dbfFd);
		}
	return dbfFd;
	}
//========================================================================
// return dos file number if AOK, else -1 (errno is set)

int RDbf::openDbf(const char* dbfname) {
	int		len;
	char*	dosname;

	len = str_len(dbfname);
	dosname = new char[len + 6];
	str_cpy(dosname, dbfname);
	if (strstr(dosname, ".sdbf") == 0)
		str_cat(dosname, ".sdbf");

#ifdef MSDOS
	_set_errno(0);
//	dbfFd = _open(dosname, _O_BINARY | _O_RDWR, _S_IREAD | _S_IWRITE);
	_sopen_s(&dbfFd, dosname, _O_BINARY | _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE);
	_get_errno(&err);
#endif
#ifdef LINUX
//	errno = 0;
	dbfFd = open(dosname, O_RDWR, S_IRUSR | S_IWUSR);
//	err = errno;
#endif
	delete[] dosname;
	if (dbfFd > 0)
		dbfBtree = new RBtree(dbfFd);

	return dbfFd;
	}
//========================================================================
// Relation oriented ops
//========================================================================
/*
DbMakeTable	- Make a new data table (relation)

int	DbMakeTable(const char* relname, RField* fldlst[]);


Creates a new data table in the specified database.

To create a new table, supply the handle of the database (dbfID)
that will contain the table, the name of the table (which must not
already exist), and descriptions for each field.  The function creates
the data relation

Fields are described by the RField class.

DbMakeRelation does not open the relation just created.

Parameters
relname - a character string of the name of the table being created
fldlst - a NULL terminated array of RField object pointers

Returns
-1 if DbMakeRelation fails

*/

int	RDbf::DbMakeTable(const char* relname, RField* fldlst[]) {
	RTable* rel;
	int		rc;

	rel = new RTable(dbUserNo, dbfFd);
	rc = 0;
	rc = rel->MakeRelation(dbfBtree, relname, fldlst);
	return rc;
	}
//========================================================================
/*
DbOpenTable	- Open a data table

RTable*	DbOpenTable(const char* tablename)

Opens the specified data Table in 'this' database
and prepares the data table for further access and modification,
returns a table object and creates a record buffer for the table.
The record buffer is initially clear. All secondary indexes are initialized
and set to UNPOSITIONED status.

To open a table, the user must be logged into the database file (*.sdbf)
containing the relation (DbLogin).

Parameters
	relname - an ascii character string of the name of the table
	being created

Returns
	On success, an RTable object is returned
	On failure, a NULL will be returned.
*/
RTable* RDbf::DbOpenTable(const char* relname) {
	RTable*	rel;
	int		rc;

	// sweep thru linked list of rel's to make sure table isn't open
	// // 240608 - allow multi use of tables
//	for (relp = dbfRelRoot; relp; relp = relp->GetLink()) {
//		if (stricmp(relname, relp->GetRelname()) == 0)
//			return (0);
//		}
	rel = new RTable(dbUserNo, dbfFd);
	rel->SetLink(dbfRelRoot);
	dbfRelRoot = rel;
	rc = rel->OpenRelation(dbfBtree, dbfRelRoot, relname);
	rc = 0;
	if (rc < 0) {
		delete rel;
		return 0;
		}
	return rel;
	}
//========================================================================
/*
DbCloseTable	- Close a Table

int	DbCloseTable(RTable* rel)

Closes the specified data relation. The RTable object is no longer valid.

Parameters

Returns
	-1 if rel is invalid
*/

int RDbf::DbCloseTable(RTable* rel) {
	RTable*	xrel;

	if (rel == NULL)
		return -1;
	// remove 'rel' from linked list of relations
	if (dbfRelRoot == rel)
		dbfRelRoot = rel->GetLink();
	else {
		for (xrel = dbfRelRoot; xrel; xrel = xrel->GetLink()) {
			if (xrel->GetLink() == rel) {
				xrel->SetLink(rel->GetLink());
				delete rel;
				break;
				}
			}
		return (-1);		// no find
		}
	return 0;
	}
//========================================================================
/*
DbDeleteTable

int	DbDeleteTable(const char* relname)

Delete all database system records from a specified database, delete all
indexes and data records.

Parameters
relname - an ascii character string of the name of the relation being created

Returns
-1 if relid is invalid

*/

int RDbf::DbDeleteTable(const char* relname) {
	RTable*	relp;
	int		rc;

	// sweep thru linked list of rel's to make sure relation isn't open
	for (relp = dbfRelRoot; relp; relp = relp->GetLink()) {
		if (str_icmp(relname, relp->GetRelname()) == 0)
			return (-1);
		}
	rc = dbfRelRoot->DropRelation(dbfBtree, relname);
	rc = 0;
	return rc;
	}
//========================================================================
int	RDbf::SetLink(RDbf* dbf) {
	dbfLink = dbf;
	return (0);
	}

void RDbf::PrintTree(int nodeno) {
	dbfBtree->PrintTree(nodeno);
	}
