#if !defined(_KEY_H_)

#define _KEY_H_

class	RKey {

	private:
		char*	keyStr;				// key string
		int		keyLen;				// key length
		
	public:
		RKey();
		~RKey();
					
		int		GetKeyLen() {return keyLen;}
		char*	GetKeyStr() {return keyStr;}
		void	GetKeyBody(const RKey &orig);
		int		GetKeyNdxNo();
		int		GetKeyRecNo();
		int		KeyAppend(RKey &otherKey);
		int		KeyCompare(RKey &otherKey);
		int		KeyCompare(const char *keyString);
					
		int		SetKey(const char *str);
		int		MakeSearchKey(const char *tmplte, ...);

		void	operator=(const RKey &other);
				RKey& operator=(const char *);
		void	PrintKey();
				
	private:
		int		Compare(const char *ikey, const char *tkey);
		int		KeyLength(const char *key);
		int		numlenl(const char *p, int l);
		int		numlenr(const char *p, int l);
   };
#endif
