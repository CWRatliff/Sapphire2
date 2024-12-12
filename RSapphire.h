#if !defined(_DB_H_)

#define _DB_H_

class DLL Sapphire {

private:
	RDbf*   dbDbfRoot;		// dbf linked list root
	int		dbUserNo;

public:
	Sapphire();
	~Sapphire();

	RDbf*	DbCreateFile(const char* dbfname);
	int		DbLogin(const char* username);
	int		DbLogout(int userno);
	RDbf*	DbUse(const char* dbfname);
	int		DbUnUse(RDbf* dbf);

	int		DbGetErrno();
	};

#if DDLLIB
int userAdd(const char* username);
void userRetire(int dbUserNo);
#endif
#endif