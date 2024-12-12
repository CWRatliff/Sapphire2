#if !defined(_DBF_H_)

#define _DBF_H_

class RDbf {

	private:
		int		dbfFd;			// DOS file #
		char*	dbfName;
		RBtree*	dbfBtree;		// Btree index for this DB
		RTable*	dbfRelRoot;		// relation linked list root
		RDbf*	dbfLink;
		int		dbUserNo;

	public:
		RDbf(const char* dbfname, int dbuserno);
		~RDbf();

		int			Create(const char* dbfname);
		DLL int		DbDeleteTable(const char* relname);
		int			openDbf(const char* dbfname);

		DLL	int		DbMakeTable(const char* relname, RField* fldlst[]);
		DLL RTable*	DbOpenTable(const char* relname);
		DLL int		DbCloseTable(RTable* rel);

		const char* GetName() { return dbfName;}
		RDbf*		GetLink() {return (dbfLink);}
		int			SetLink(RDbf* dbf);

		DLL void	PrintTree(int nodeno = 0);
	};
#endif