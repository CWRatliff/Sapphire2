#if !defined(_INDEX_H_)

#define _INDEX_H_

class RIndex {

	private:
//		char*	ndxName;
		int		ndxNo;				// index number
		int		ndxFldCnt;			// number of fields in key
		RField*	ndxFld[MAXKEY+1];	// array of RField pointers
		int		ndxType[MAXKEY+1];	// array of field:key types
		RBtree*	ndxBTree;
		RIndex*	ndxLink;			// linked list pointer
		NDX_ID	ndx_ID;				// where we are right now

	public:
		char*	ndxName;
		RIndex();
		RIndex(const char* ndxname, int ndxno, RBtree *ndxbtree);
		~RIndex();

		int		AddField(RField& fld, int desc);
		int		Add(int recno);
		int		Delete();
		int		Fetch(RField* fldlst[]);
		int		Fetch(int recno, RField* fldlst[]);

		int		Find(RKey &key);
		int		First();
		int		Last();
		int		Next(int ndxtype = 1);
		int		Prev(int ndxtype = 1);

		int		GetCurrentKey(RKey* key);
		RIndex*	GetIndexHandle(const char* ndxname);
		int		GetLastRecNo();
		int		GetNdxNo() {return(ndxNo);}
		int		GetFieldCount() {return (ndxFldCnt);}
		RIndex* GetLink() {return (ndxLink);}
		int		GetRecno();
		int		GetStatus() {return(ndx_ID.ndxStatus);}

		int		PresetIndexes(int recno);
		void	Reset() {ndxFldCnt = 0;}
		int		SetLink(RIndex* ndx);
		int		UnLink(RIndex* ndx);
		int		UpdateIndex();
		int		WriteData(RData &data, int recno);
		void	PrintIndex();
	};
#endif