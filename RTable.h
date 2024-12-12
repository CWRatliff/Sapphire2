#if !defined(_RELATION_H_)

#define _RELATION_H_

#if DLLLIB
int recordLock(int usr, int fd, int tab, int recno);
int recordUnlock(int usr, int fd, int tab, int recno);
int recordQuery(int usr, int fd, int tab, int recno);
#endif

class RTable {

	private:
		char*	relName;
		int		relFd;				// dbf file descriptor
		int		relUserNo;			// user number
		int		relnFlds;			// # flds in table
		int		relNdxNo;			// data records index No
		int		relRecNo;			// data record number
		int		relRecStatus;		// status of current record
		int		relOwner;			// TRUE if 'this' allocated relFldLst
		RIndex*	relDataNdx;			// data item index
		RIndex*	relCurNdx;			// index last used for position
		RIndex*	relNdxRoot;			// linked list of indexes
		RField** relFldLst;			// array of Field object ptrs
		RBtree*	relNdx;				// Btree associated
		RTable*	relLink;			// linked list pointer
		RTable* relRootPtr;			// linked list root

	public:
		RTable(int userno, int fd);
		~RTable();

		int		MakeRelation(RBtree* btree, const char *relname, RField *fldlst[]);
		int		OpenRelation(RBtree* btree, RTable* relRoot, const char* relname);
		int		DropRelation(RBtree* btree, const char *relname);

		DLL	int	DbLockRecord();
		DLL int	DbUnlockRecord();
		DLL int	DbLockQuery();
//		DLL int	DbLockTable();
//		DLL int	DbUnlockTable();

		DLL int	DbAddRecord();
		DLL int	DbClearRecord();
		DLL int	DbDeleteRecord();
		DLL int	DbRefreshRecord();
		DLL int	DbUpdateRecord();

		DLL int	DbFirstRecord(RIndex* ndx = 0);
		DLL int	DbNextRecord(RIndex* ndx = 0);
		DLL int	DbLastRecord(RIndex* ndx = 0);
		DLL int	DbPrevRecord(RIndex* ndx = 0);
		DLL int	DbSetIndex(RIndex* ndx = 0);
		DLL int	DbSearchRecord(RIndex* ndx, const char* key);

		DLL RField*	DbGetFieldObject(const char* fldname, int offset = 0);

		DLL const char* DbGetCharPtr(const char* fldname, int offset = 0);
		DLL int	DbGetCharCopy(const char* fldname, char* data, int len, int offset);
		DLL int	DbGetInt(const char* fldname, int offset = 0);
		DLL float DbGetFloat(const char* fldname, int offset = 0);
		DLL double DbGetDouble(const char* fldname, int offset = 0);

		DLL int	DbSetField(const char* fldname, const char* data, int offset = 0);
		DLL int	DbSetField(const char* fldname, int data, int offset = 0);
		DLL int	DbSetField(const char* fldname, float data, int offset = 0);
		DLL int	DbSetField(const char* fldname, double data, int offset = 0);

		DLL int		DbDeleteIndex(const char* ndxname);
		DLL RIndex*	DbMakeIndex(const char* ndxname, const char *tmplte, RField *fldlst[]);
		DLL RIndex*	DbGetIndexObject(const char* ndxname);
		int		SetLink(RTable* rel);
		RTable*	GetLink() {return (relLink);}
		char*	GetRelname() {return (relName);}
		int		GetRecno();
		void	PrintIndexList();
	private:
		int		AddIndex(RIndex* ndx);
		int		AnyFieldChanged();
		int		GetHighestIndex(RIndex &ndxrel, RIndex &ndxndx);
		int		DeleteFields();
		int		DeleteIndexes();
		RField*	ScanFieldList(const char* fldname, int offset = 0);
		void	ResetIndexes();

};
#endif