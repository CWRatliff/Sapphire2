#if !defined(_DB_H_)

#define _DB_H_

class Sapphire {

	private:
		RDbf*	dbDbfRoot;		// dbf linked list root

	public:
		Sapphire();
		~Sapphire();

		RDbf*	DbCreateFile(const char* dbfname);
		RDbf*	DbUse(const char* dbfname);
		int		DbUnUse(RDbf* dbf);
		
		int		DbGetErrno();
	};
#endif