#if !defined(_FIELD_H_)

#define _FIELD_H_

//#define	DLL __declspec(dllexport)
#define DLL
class DLL RField {

	private:
		char*	fldName;
		char*	fldData;		// data item
		int		fldType;		// ref ndbdefs.h
		int		fldLen;			// max fld length
		int		fldChg;			// status: bit 0 - change (ON = changed)
		int		fldOwner;		// data area owned by object -> TRUE

	public:
		RField();
		RField(const char* fldname, char* flddata, int fldtype, int fldlen);
		RField(const char* fldname, int fldtype, int fldlen);
		RField(const char* fldname, int fldtype);
		~RField();

		int		ClearField();

		char*	GetName() {return (fldName);}
		int		GetLen() {return (fldLen);}
		int		GetType() {return (fldType);}
		char*	GetDataAddr() {return (fldData); }

		const char*	GetCharPtr();
		int		GetCharCopy(char* data, int len);
		int		GetInt();
		float	GetFloat();
		double	GetDouble();

		int		SetData(const char *data);
		int		SetData(int data);
		int		SetData(float data);
		int		SetData(double data);

		void	ClearFieldStatus() {fldChg &= !1;}
		int		GetFieldStatus() {return (fldChg);}
		void	SetFieldStatus() {fldChg &= 1;}
	};
/*
class RFieldC : public RField {
	public:
		RFieldC(const char* fldname, char* flddata, int fldtype, int fldlen) : RField(fldname, flddata, fldtype, fldlen) {}
		RFieldC(const char* fldname, char* flddata, int fldlen) : RField(fldname, flddata, STRING, fldlen) {}
		RFieldC(const char* fldname, int fldtype, int fldlen) : RField(fldname, fldtype, fldlen) {}
		const char*	Get();
		int GetCopy(char* data, int len);
	};
class RFieldI : public RField {
	public:
		RFieldI(const char* fldname) : RField(fldname, INT) {}
		RFieldI(const char* fldname, char* flddata) : RField(fldname, flddata, INT, sizeof(int)) {}
		int		Get();
	};
class RFieldF : public RField {
	public:
		RFieldF(const char* fldname) : RField(fldname, FP) {}
		RFieldF(const char* fldname, char* flddata) : RField(fldname, flddata, FP, sizeof(float)) {}
		float	Get();
	};
class RFieldD : public RField {
	public:
		RFieldD(const char* fldname) : RField(fldname, DP) {}
		RFieldD(const char* fldname, char* flddata) : RField(fldname, flddata, DP, sizeof(double)) {}
		double	Get();
	};

//=============================================================================
// fetch a char pointer to character type field data
const char* RFieldC::Get() {
	const char* p;

	p = GetDataAddr();
	return (p);
	}
//=============================================================================
// fetch a char pointer to character type field data
int RFieldC::GetCopy(char* data, int len) {
	char*	p;
	int		flen;

	p = fldData;
	flen = fldLen;
	if (len > flen)
		len = flen;
	strncpy(p, data, len);
	data[len - 1] = '\0';
	return len;
	}
//=============================================================================
// fetch an int field
int RFieldI::Get() {
	int		i = 0;
	const char* p;

	p = GetDataAddr();
	memcpy(&i, p, sizeof(int));
	return (i);
	}
//=============================================================================
// fetch a float field
float RFieldF::Get() {
	float		f = 0;
	const char* p;

	p = GetDataAddr();
	memcpy(&f, p, sizeof(float));
	return (f);
	}
//=============================================================================
// fetch a double field
	double RFieldD::Get() {
	double		d = 0;
	const char* p;

	p = GetDataAddr();
	memcpy(&d, p, sizeof(double));
	return (d);
	}
*/
#endif