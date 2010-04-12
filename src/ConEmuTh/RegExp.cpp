/*
  Copyright (c) 2000 Konstantin Stupnik
  Copyright (c) 2008 Far Group
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. The name of the authors may not be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Regular expressions support library.
  Syntax and semantics of regexps very close to
  syntax and semantics of perl regexps.
*/

#include "headers.hpp"
#pragma hdrstop

#include "RegExp.hpp"

#ifndef RE_FAR_MODE

#ifndef UNICODE
#ifndef RE_EXTERNAL_CTYPE
#include <ctype.h>
#endif
#else
#ifndef __LINUX
#include <windows.h>
#endif
#endif
#ifndef RE_NO_STRING_H
#include <string.h>
#endif

#else

#define malloc xf_malloc
#define free xf_free

#endif

#ifdef RE_DEBUG
#include <stdio.h>
#ifdef dpf
#undef dpf
#endif
#define dpf(x) printf x

char *ops[]=
{
	"opNone",
	"opLineStart",
	"opLineEnd",
	"opDataStart",
	"opDataEnd",
	"opWordBound",
	"opNotWordBound",
	"opType",
	"opNotType",
	"opCharAny",
	"opCharAnyAll",
	"opSymbol",
	"opNotSymbol",
	"opSymbolIgnoreCase",
	"opNotSymbolIgnoreCase",
	"opSymbolClass",
	"opOpenBracket",
	"opClosingBracket",
	"opAlternative",
	"opBackRef",
#ifdef NAMEDBRACKETS
	"opNamedBracket",
	"opNamedBackRef",
#endif
	"opRangesBegin",
	"opRange",
	"opMinRange",
	"opSymbolRange",
	"opSymbolMinRange",
	"opNotSymbolRange",
	"opNotSymbolMinRange",
	"opAnyRange",
	"opAnyMinRange",
	"opTypeRange",
	"opTypeMinRange",
	"opNotTypeRange",
	"opNotTypeMinRange",
	"opClassRange",
	"opClassMinRange",
	"opBracketRange",
	"opBracketMinRange",
	"opBackRefRange",
	"opBackRefMinRange",
#ifdef NAMEDBRACKETS
	"opNamedRefRange",
	"opNamedRefMinRange",
#endif
	"opRangesEnd",
	"opAssertionsBegin",
	"opLookAhead",
	"opNotLookAhead",
	"opLookBehind",
	"opNotLookBehind",
	"opAsserionsEnd",
	"opNoReturn",
#ifdef RELIB
	"opLibCall",
#endif
	"opRegExpEnd",
};

#else
#define dpf(x)
#endif

#ifndef UNICODE
#ifdef RE_STATIC_LOCALE
#ifdef RE_EXTERNAL_CTYPE
prechar RegExp::lc;
prechar RegExp::uc;
prechar RegExp::chartypes;
#else
int RegExp::ilc[256/sizeof(int)];
int RegExp::iuc[256/sizeof(int)];
int RegExp::ichartypes[256/sizeof(int)];
rechar* RegExp::lc=(rechar*)RegExp::ilc;
rechar* RegExp::uc=(rechar*)RegExp::iuc;
rechar* RegExp::chartypes=(rechar*)RegExp::ichartypes;
#endif
int RegExp::icharbits[256/sizeof(int)];
rechar* RegExp::charbits=(rechar*)RegExp::icharbits;
#endif
#endif

#ifdef UNICODE
#ifndef __LINUX

#define ISDIGIT(c) iswdigit(c)
#define ISSPACE(c) iswspace(c)
#define ISWORD(c)  (IsCharAlphaNumeric(c) || c=='_')
#define ISLOWER(c) IsCharLower(c)
#define ISUPPER(c) IsCharUpper(c)
#define ISALPHA(c) IsCharAlpha(c)
#define TOUPPER(c) ((rechar)(DWORD_PTR)CharUpper((LPTSTR)(DWORD_PTR)c))
#define TOLOWER(c) ((rechar)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)c))

#else

#define ISDIGIT(c) iswdigit(c)
#define ISSPACE(c) iswspace(c)
#define ISWORD(c)  (iswalnum(c) || c=='_')
#define ISLOWER(c) iswlower(c)
#define ISUPPER(c) iswupper(c)
#define ISALPHA(c) iswalpha(c)
#define TOUPPER(c) towupper(c)
#define TOLOWER(c) towlower(c)

#endif

#define ISTYPE(c,t) isType(c,t)

int isType(rechar chr,int type)
{
	switch (type)
	{
		case TYPE_DIGITCHAR:return ISDIGIT(chr);
		case TYPE_SPACECHAR:return ISSPACE(chr);
		case TYPE_WORDCHAR: return ISWORD(chr);
		case TYPE_LOWCASE:  return ISLOWER(chr);
		case TYPE_UPCASE:   return ISUPPER(chr);
		case TYPE_ALPHACHAR:return ISALPHA(chr);
	}

	return false;
}

int ushlen(const rechar* str)
{
	rechar ch;
	int len = -1;

	do
	{
		ch = str[len+1];
		len++;
	}
	while (ch!=0);

	return len;
}

#define strlen ushlen

struct UniSet
{
	unsigned char* high[256];
	char types;
	char nottypes;
	char negative;
	UniSet()
	{
		memset(high,0,sizeof(high));
		types=0;
		nottypes=0;
		negative=0;
	}
	UniSet(const UniSet& src)
	{
		for (int i=0; i<256; i++)
		{
			if (src.high[i])
			{
				high[i]=new unsigned char[32];
				memcpy(high[i],src.high[i],32);
			}
			else
			{
				high[i]=nullptr;
			}
		}

		types=src.types;
		nottypes=src.nottypes;
		negative=src.negative;
	}
	UniSet& operator=(const UniSet& src)
	{
		if (this != &src)
		{
			for (int i=0; i<256; i++)
			{
				if (src.high[i])
				{
					if (!high[i])high[i]=new unsigned char[32];

					memcpy(high[i],src.high[i],32);
				}
				else
				{
					if (high[i])delete [] high[i];

					high[i]=nullptr;
				}
			}

			types=src.types;
			nottypes=src.nottypes;
			negative=src.negative;
		}

		return (*this);
	}

	void Reset()
	{
		for (int i=0; i<256; i++)
		{
			if (high[i])
			{
				delete [] high[i];
				high[i]=0;
			}
		}

		types=0;
		nottypes=0;
		negative=0;
	}

	struct Setter
	{
		UniSet& set;
		rechar idx;
		Setter(UniSet& s,rechar chr):set(s),idx(chr)
		{
		}
		void operator=(int val)
		{
			if (val)set.SetBit(idx);
			else set.ClearBit(idx);
		}
		bool operator!()const
		{
			return !set.GetBit(idx);
		}
	};

	const bool operator[](rechar idx)const
	{
		return GetBit(idx);
	}
	Setter operator[](rechar idx)
	{
		return Setter(*this,idx);
	}
	~UniSet()
	{
		for (int i=0; i<256; i++)
		{
			if (high[i])delete [] high[i];
		}
	}
	bool CheckType(int t, rechar chr) const
	{
		switch (t)
		{
			case TYPE_DIGITCHAR:if (ISDIGIT(chr))return true; else break;
			case TYPE_SPACECHAR:if (ISSPACE(chr))return true; else break;
			case TYPE_WORDCHAR: if (ISWORD(chr)) return true; else break;
			case TYPE_LOWCASE:  if (ISLOWER(chr))return true; else break;
			case TYPE_UPCASE:   if (ISUPPER(chr))return true; else break;
			case TYPE_ALPHACHAR:if (ISALPHA(chr))return true; else break;
		}

		return false;
	}
	bool GetBit(rechar chr) const
	{
		if (types)
		{
			int t=TYPE_ALPHACHAR;

			while (t)
			{
				if (types&t)
				{
					if (CheckType(t,chr))
						return negative?false:true;
				}

				t>>=1;
			}
		}

		if (nottypes)
		{
			int t=TYPE_ALPHACHAR;

			while (t)
			{
				if (nottypes&t)
				{
					if (!CheckType(t,chr))
						return negative?false:true;
				}

				t>>=1;
			}
		}

		unsigned char h=(chr&0xff00)>>8;

		if (!high[h]) return negative?true:false;

		if (((high[h][(chr&0xff)>>3]&(1<<(chr&7)))!=0?1:0))
		{
			return negative?false:true;
		}

		return negative?true:false;
	}
	void SetBit(rechar  chr)
	{
		unsigned char h=(chr&0xff00)>>8;

		if (!high[h])
		{
			high[h]=new unsigned char[32];
			memset(high[h],0,32);
		}

		high[h][(chr&0xff)>>3]|=1<<(chr&7);
	}
	void ClearBit(rechar  chr)
	{
		unsigned char h=(chr&0xff00)>>8;

		if (!high[h])
		{
			high[h]=new unsigned char[32];
			memset(high[h],0,32);
		}

		high[h][(chr&0xff)>>3]&=~(1<<(chr&7));
	}

};

#define GetBit(cls,chr) cls->GetBit(chr)
#define SetBit(cls,chr) cls->SetBit(chr)

#else
#define ISDIGIT(c) ((chartypes[c]&TYPE_DIGITCHAR)!=0)
#define ISSPACE(c) ((chartypes[c]&TYPE_SPACECHAR)!=0)
#define ISWORD(c)  ((chartypes[c]&TYPE_WORDCHAR)!=0)
#define ISLOWER(c) ((chartypes[c]&TYPE_LOWCASE)!=0)
#define ISUPPER(c) ((chartypes[c]&TYPE_UPCASE)!=0)
#define ISALPHA(c) ((chartypes[c]&TYPE_ALPHACHAR)!=0)
#define TOUPPER(c) uc[c]
#define TOLOWER(c) lc[c]

#define ISTYPE(c,t) (chartypes[c]&t)

#endif //UNICODE



enum REOp
{
	opLineStart=0x1,        // ^
	opLineEnd,              // $
	opDataStart,            // \A and ^ in single line mode
	opDataEnd,              // \Z and $ in signle line mode

	opWordBound,            // \b
	opNotWordBound,         // \B

	opType,                 // \d\s\w\l\u\e
	opNotType,              // \D\S\W\L\U\E

	opCharAny,              // .
	opCharAnyAll,           // . in single line mode

	opSymbol,               // single char
	opNotSymbol,            // [^c] negative charclass with one char
	opSymbolIgnoreCase,     // symbol with IGNORE_CASE turned on
	opNotSymbolIgnoreCase,  // [^c] with ignore case set.

	opSymbolClass,          // [chars]

	opOpenBracket,          // (

	opClosingBracket,       // )

	opAlternative,          // |

	opBackRef,              // \1

#ifdef NAMEDBRACKETS
	opNamedBracket,         // (?{name}
	opNamedBackRef,         // \p{name}
#endif


	opRangesBegin,          // for op type check

	opRange,                // generic range
	opMinRange,             // generic minimizing range

	opSymbolRange,          // quantifier applied to single char
	opSymbolMinRange,       // minimizing quantifier

	opNotSymbolRange,       // [^x]
	opNotSymbolMinRange,

	opAnyRange,             // .
	opAnyMinRange,

	opTypeRange,            // \w, \d, \s
	opTypeMinRange,

	opNotTypeRange,         // \W, \D, \S
	opNotTypeMinRange,

	opClassRange,           // for char classes
	opClassMinRange,

	opBracketRange,         // for brackets
	opBracketMinRange,

	opBackRefRange,         // for backrefs
	opBackRefMinRange,

#ifdef NAMEDBRACKETS
	opNamedRefRange,
	opNamedRefMinRange,
#endif

	opRangesEnd,            // end of ranges

	opAssertionsBegin,

	opLookAhead,
	opNotLookAhead,

	opLookBehind,
	opNotLookBehind,

	opAsserionsEnd,

	opNoReturn,

#ifdef RELIB
	opLibCall,
#endif
	opRegExpEnd
};

struct REOpCode
{
	int op;
	REOpCode *next,*prev;
#ifdef RE_DEBUG
	int    srcpos;
#endif
#ifdef RE_NO_NEWARRAY
	static void OnCreate(void *ptr);
	static void OnDelete(void *ptr);
#else
	REOpCode()
	{
		memset(this,0,sizeof(*this));
	}
	~REOpCode();
#endif

	struct SBracket
	{
		REOpCode* nextalt;
		int index;
		REOpCode* pairindex;
	};

	struct SRange
	{
		union
		{
			SBracket bracket;
			int op;
			rechar symbol;
#ifdef UNICODE
			UniSet *symbolclass;
#else
			prechar symbolclass;
#endif
			REOpCode* nextalt;
			int refindex;
#ifdef NAMEDBRACKETS
			prechar refname;
#endif
			int type;
		};
		int min,max;
	};

	struct SNamedBracket
	{
		REOpCode* nextalt;
		prechar name;
		REOpCode* pairindex;
	};

	struct SAssert
	{
		REOpCode* nextalt;
		int length;
		REOpCode* pairindex;
	};

	struct SAlternative
	{
		REOpCode* nextalt;
		REOpCode* endindex;
	};


	union
	{
		SRange range;
		SBracket bracket;
#ifdef NAMEDBRACKETS
		SNamedBracket nbracket;
#endif
		SAssert assert;
		SAlternative alternative;
		rechar symbol;
#ifdef UNICODE
		UniSet *symbolclass;
#else
		prechar symbolclass;
#endif
		int refindex;
#ifdef NAMEDBRACKETS
		prechar refname;
#endif
#ifdef RELIB
		prechar rename;
#endif

		int type;
	};
};

#ifdef RE_NO_NEWARRAY
void StateStackItem::OnCreate(void *ptr)
{
	memset(ptr,0,sizeof(StateStackItem));
}

void REOpCode::OnCreate(void *ptr)
{
	memset(ptr,0,sizeof(REOpCode));
}

void REOpCode::OnDelete(void *ptr)
{
	REOpCode &o=*static_cast<REOpCode*>(ptr);

	switch (o.op)
	{
		case opSymbolClass:

			if (o.symbolclass)
				free(o.symbolclass);

			break;
		case opClassRange:
		case opClassMinRange:

			if (o.range.symbolclass)
				free(o.range.symbolclass);

			break;
#ifdef NAMEDBRACKETS
		case opNamedBracket:

			if (o.nbracket.name)
				free(o.nbracket.name);

			break;
		case opNamedBackRef:

			if (o.refname)
				free(o.refname);

			break;
#endif
#ifdef RELIB
		case opLibCall:

			if (o.rename)
				free(o.rename);

			break;
#endif
	}
}

void *RegExp::CreateArray(const unsigned int size, const unsigned int total,
                          ON_CREATE_FUNC Create)
{
	if (total && size)
	{
		/* record[0] - sizeof
		   record[1] - total
		   record[2] - array
		*/
		unsigned char *record=static_cast<unsigned char*>
		                      (malloc(sizeof(unsigned int)*2+size*total));

		if (record)
		{
			unsigned char *array=record+2*sizeof(unsigned int);
			*reinterpret_cast<int*>(record)=size;
			*reinterpret_cast<int*>(record+sizeof(unsigned int))=total;

			if (Create!=nullptr)
				for (unsigned int f=0; f<total; ++f)
					Create(array+size*f);

			return reinterpret_cast<void*>(array);
		}
	}

	return nullptr;
}

void RegExp::DeleteArray(void **array, ON_DELETE_FUNC Delete)
{
	if (array && *array)
	{
		unsigned char *record=reinterpret_cast<unsigned char*>(*array)-
		                      2*sizeof(unsigned int);

		if (Delete!=nullptr)
		{
			unsigned char *m=static_cast<unsigned char*>(*array);
			unsigned int size=*reinterpret_cast<int*>(record),
			                  total=*reinterpret_cast<int*>(record+sizeof(unsigned int));

			for (unsigned int f=0; f<total; ++f)
				Delete(m+size*f);
		}

		free(record);
		*array=nullptr;
	}
}
#else  // RE_NO_NEWARRAY
REOpCode::~REOpCode()
{
	switch (op)
	{
#ifdef UNICODE
		case opSymbolClass:delete symbolclass; break;
#else
case opSymbolClass:delete [] symbolclass; break;
#endif
#ifdef UNICODE
		case opClassRange:
		case opClassMinRange:delete range.symbolclass; break;
#else
case opClassRange:
case opClassMinRange:delete [] range.symbolclass; break;
#endif
#ifdef NAMEDBRACKETS
		case opNamedBracket:delete [] nbracket.name; break;
		case opNamedBackRef:delete [] refname; break;
#endif
#ifdef RELIB
		case opLibCall:delete [] rename; break;
#endif
	}
}
#endif // RE_NO_NEWARRAY



void RegExp::Init(const prechar expr,int options)
{
	//memset(this,0,sizeof(*this));
	code=nullptr;
	brhandler=nullptr;
	brhdata=nullptr;
#ifndef UNICODE
#ifndef RE_STATIC_LOCALE
#ifndef RE_EXTERNAL_CTYPE
	InitLocale();
#endif //RE_EXTERNAL_CTYPE
#endif//RE_STATIC_LOCALE
#endif //UNICODE
#ifdef NAMEDBRACKETS
	havenamedbrackets=0;
#endif
	stack=&initstack[0];
	st=&stack[0];
	initstackpage.stack=stack;
	firstpage=lastpage=&initstackpage;
	firstpage->next=nullptr;
	firstpage->prev=nullptr;
#ifdef UNICODE
	firstptr=new UniSet();
#define first (*firstptr)
#endif
	start=nullptr;
	end=nullptr;
	trimend=nullptr;
	Compile((const RECHAR*)expr,options);
}

RegExp::RegExp():
	code(nullptr),
#ifdef NAMEDBRACKETS
	havenamedbrackets(0),
#endif
	stack(&initstack[0]),
	st(&stack[0]),
	slashChar('/'),
	backslashChar('\\'),
	firstpage(&initstackpage),
	lastpage(&initstackpage),
#ifdef UNICODE
	firstptr(new UniSet()),
#endif
	errorcode(errNotCompiled),
	start(nullptr),
	end(nullptr),
	trimend(nullptr),
#ifdef RE_DEBUG
	resrc(nullptr),
#endif
	brhandler(nullptr),
	brhdata(nullptr)
{
#ifndef UNICODE
#ifndef RE_STATIC_LOCALE
#ifndef RE_EXTERNAL_CTYPE
	InitLocale();
#endif
#endif
#endif//UNICODE
	initstackpage.stack=stack;
	firstpage->next=nullptr;
	firstpage->prev=nullptr;
}

RegExp::RegExp(const RECHAR* expr,int options)
{
	slashChar='/';
	backslashChar='\\';
#ifdef RE_DEBUG
	resrc=nullptr;
#endif
	Init((const prechar)expr,options);
}

RegExp::~RegExp()
{
#ifdef RE_DEBUG
#ifdef RE_NO_NEWARRAY

	if (resrc)
		free(resrc);

#else
	delete [] resrc;
#endif // RE_NO_NEWARRAY
#endif

	if (code)
	{
#ifdef RE_NO_NEWARRAY
		DeleteArray(reinterpret_cast<void**>(&code),REOpCode::OnDelete);
#else
		delete [] code;
		code=nullptr;
#endif
	}

	CleanStack();
#ifdef UNICODE
	delete firstptr;
#endif
}

#ifndef UNICODE
#ifndef RE_EXTERNAL_CTYPE
void RegExp::InitLocale()
{
	for (int i=0; i<256; i++)
	{
		lc[i]=tolower(i);
		uc[i]=toupper(i);
	}

	for (int i=0; i<256; i++)
	{
		char res=0;

		if (isalnum(i) || i=='_')res|=TYPE_WORDCHAR;

		if (isalpha(i))res|=TYPE_ALPHACHAR;

		if (isdigit(i))res|=TYPE_DIGITCHAR;

		if (isspace(i))res|=TYPE_SPACECHAR;

		if (lc[i]==i && uc[i]!=i)res|=TYPE_LOWCASE;

		if (uc[i]==i && lc[i]!=i)res|=TYPE_UPCASE;

		chartypes[i]=res;
	}

	memset(charbits,0,sizeof(charbits));

	for (int i=0,j=0,k=1; i<256; i++)
	{
		if (chartypes[i]&TYPE_DIGITCHAR) {charbits[j]|=k;}

		if (chartypes[i]&TYPE_SPACECHAR) {charbits[32+j]|=k;}

		if (chartypes[i]&TYPE_WORDCHAR) {charbits[64+j]|=k;}

		if (chartypes[i]&TYPE_LOWCASE) {charbits[96+j]|=k;}

		if (chartypes[i]&TYPE_UPCASE) {charbits[128+j]|=k;}

		if (chartypes[i]&TYPE_ALPHACHAR) {charbits[160+j]|=k;}

		k<<=1;

		if (k==256) {k=1; j++;}
	}
}
#endif
#endif



int RegExp::CalcLength(const prechar src,int srclength)
{
	int length=3;//global brackets
	int brackets[MAXDEPTH];
	int count=0;
	int i,save;
	bracketscount=1;
	int inquote=0;

	for (i=0; i<srclength; i++,length++)
	{
		if (inquote && src[i]!=backslashChar && src[i+1]!='E')
		{
			continue;
		}

		if (src[i]==backslashChar)
		{
			i++;

			if (src[i]=='Q')inquote=1;

			if (src[i]=='E')inquote=0;

			if (src[i]=='x')
			{
				i++;
				if(isxdigit(src[i]))
				{
					for(int j=1,k=i;j<4;j++)
					{
						if(isxdigit(src[k+j]))
						{
							i++;
						}
						else
						{
							break;
						}
					}
				}
				else return SetError(errSyntax,i);
			}

#ifdef NAMEDBRACKETS

			if (src[i]=='p')
			{
				i++;

				if (src[i]!='{')
					return SetError(errSyntax,i);

				i++;
				int save2=i;

				while (i<srclength && (ISWORD(src[i]) || ISSPACE(src[i])) && src[i]!='}')
					i++;

				if (i>=srclength)
					return SetError(errBrackets,save2);

				if (src[i]!='}' && !(ISWORD(src[i]) || ISSPACE(src[i])))
					return SetError(errSyntax,i);
			}

#endif
			continue;
		}

		switch (src[i])
		{
			case '(':
			{
				brackets[count]=i;
				count++;

				if (count==MAXDEPTH)return SetError(errMaxDepth,i);

				if (src[i+1]=='?')
				{
					i+=2;
#ifdef NAMEDBRACKETS

					if (src[i]=='{')
					{
						save=i;
						i++;

						while (i<srclength && (ISWORD(src[i]) || ISSPACE(src[i])) && src[i]!='}')
							i++;

						if (i>=srclength)
							return SetError(errBrackets,save);

						if (src[i]!='}' && !(ISWORD(src[i]) || ISSPACE(src[i])))
							return SetError(errSyntax,i);
					}

#endif
				}
				else
				{
					bracketscount++;
				}

				break;
			}
			case ')':
			{
				count--;

				if (count<0)return SetError(errBrackets,i);

				break;
			}
			case '{':
			case '*':
			case '+':
			case '?':
			{
				length--;

				if (src[i]=='{')
				{
					save=i;

					while (i<srclength && src[i]!='}')i++;

					if (i>=srclength)return SetError(errBrackets,save);
				}

				if (src[i+1]=='?')i++;

				break;
			}
			case '[':
			{
				save=i;

				while (i<srclength && src[i]!=']')i++;

				if (i>=srclength)return SetError(errBrackets,save);

				break;
			}
#ifdef RELIB
			case '%':
			{
				i++;
				save=i;

				while (i<srclength && src[i]!='%')i++;

				if (i>=srclength)return SetError(errBrackets,save-1);

				if (save==i)return SetError(errSyntax,save);
			} break;
#endif
		}
	}

	if (count)
	{
		errorpos=brackets[0];
		errorcode=errBrackets;
		return 0;
	}

	return length;
}

int RegExp::Compile(const RECHAR* src,int options)
{
	int srcstart=0,srclength/*=0*/,relength;

	if (options&OP_CPPMODE)
	{
		slashChar='\\';
		backslashChar='/';
	}
	else
	{
		slashChar='/';
		backslashChar='\\';
	}

	havefirst=0;
#ifdef RE_NO_NEWARRAY
	DeleteArray(reinterpret_cast<void**>(&code),REOpCode::OnDelete);
#else

	if (code)delete [] code;

	code=nullptr;
#endif

	if (options&OP_PERLSTYLE)
	{
		if (src[0]!=slashChar)return SetError(errSyntax,0);

		srcstart=1;
		srclength=1;

		while (src[srclength] && src[srclength]!=slashChar)
		{
			if (src[srclength]==backslashChar && src[srclength+1]!=0)
			{
				srclength++;
			}

			srclength++;
		}

		if (!src[srclength])
		{
			return SetError(errSyntax,srclength-1);
		}

		int i=srclength+1;
		srclength--;

		while (src[i])
		{
			switch (src[i])
			{
				case 'i':options|=OP_IGNORECASE; break;
				case 's':options|=OP_SINGLELINE; break;
				case 'm':options|=OP_MULTILINE; break;
				case 'x':options|=OP_XTENDEDSYNTAX; break;
				case 'o':options|=OP_OPTIMIZE; break;
				default:return SetError(errOptions,i);
			}

			i++;
		}
	}
	else
	{
		srclength=(int)strlen(src);
	}

	ignorecase=options&OP_IGNORECASE?1:0;
	relength=CalcLength((const prechar)src+srcstart,srclength);

	if (relength==0)
	{
		return 0;
	}

#ifdef RE_NO_NEWARRAY
	code=static_cast<REOpCode*>
	     (CreateArray(sizeof(REOpCode), relength, REOpCode::OnCreate));
#else
	code=new REOpCode[relength];
	memset(code,0,sizeof(REOpCode)*relength);
#endif

	for (int i=0; i<relength; i++)
	{
		code[i].next=i<relength-1?code+i+1:0;
		code[i].prev=i>0?code+i-1:0;
	}

	int result=InnerCompile((const prechar)src+srcstart,srclength,options);

	if (!result)
	{
#ifdef RE_NO_NEWARRAY
		DeleteArray(reinterpret_cast<void**>(&code),REOpCode::OnDelete);
#else
		delete [] code;
		code=nullptr;
#endif
	}
	else
	{
		errorcode=errNone;
		minlength=0;

		if (options&OP_OPTIMIZE)Optimize();
	}

	return result;
}

int RegExp::GetNum(const prechar src,int& i)
{
	int res=0;//atoi((const char*)src+i);

	while (ISDIGIT(src[i]))
	{
		res*=10;
		res+=src[i]-'0';
		i++;
	}

	return res;
}

static int CalcPatternLength(PREOpCode from,PREOpCode to)
{
	int len=0;
	int altcnt=0;
	int altlen=-1;

	for (; from->prev!=to; from=from->next)
	{
		switch (from->op)
		{
				//zero width
			case opLineStart:
			case opLineEnd:
			case opDataStart:
			case opDataEnd:
			case opWordBound:
			case opNotWordBound:continue;
			case opType:
			case opNotType:
			case opCharAny:
			case opCharAnyAll:
			case opSymbol:
			case opNotSymbol:
			case opSymbolIgnoreCase:
			case opNotSymbolIgnoreCase:
			case opSymbolClass:
				len++;
				altcnt++;
				continue;
#ifdef NAMEDBRACKETS
			case opNamedBracket:
#endif
			case opOpenBracket:
			{
				int l=CalcPatternLength(from->next,from->bracket.pairindex->prev);

				if (l==-1)return -1;

				len+=l;
				altcnt+=l;
				from=from->bracket.pairindex;
				continue;
			}
			case opClosingBracket:
				break;
			case opAlternative:

				if (altlen!=-1 && altcnt!=altlen)return -1;

				altlen=altcnt;
				altcnt=0;
				continue;
			case opBackRef:
#ifdef NAMEDBRACKETS
			case opNamedBackRef:
#endif
				return -1;
			case opRangesBegin:
			case opRange:
			case opMinRange:
			case opSymbolRange:
			case opSymbolMinRange:
			case opNotSymbolRange:
			case opNotSymbolMinRange:
			case opAnyRange:
			case opAnyMinRange:
			case opTypeRange:
			case opTypeMinRange:
			case opNotTypeRange:
			case opNotTypeMinRange:
			case opClassRange:
			case opClassMinRange:

				if (from->range.min!=from->range.max)return -1;

				len+=from->range.min;
				altcnt+=from->range.min;
				continue;
			case opBracketRange:
			case opBracketMinRange:
			{
				if (from->range.min!=from->range.max)return -1;

				int l=CalcPatternLength(from->next,from->bracket.pairindex->prev);

				if (l==-1)return -1;

				len+=from->range.min*l;
				altcnt+=from->range.min*l;
				from=from->bracket.pairindex;
				continue;
			}
			case opBackRefRange:
			case opBackRefMinRange:
#ifdef NAMEDBRACKETS
			case opNamedRefRange:
			case opNamedRefMinRange:
#endif
				return -1;
			case opRangesEnd:
			case opAssertionsBegin:
			case opLookAhead:
			case opNotLookAhead:
			case opLookBehind:
			case opNotLookBehind:
				from=from->assert.pairindex;
				continue;
			case opAsserionsEnd:
			case opNoReturn:
				continue;
#ifdef RELIB
			case opLibCall:
				return -1;
#endif
		}
	}

	if (altlen!=-1 && altlen!=altcnt)return -1;

	return altlen==-1?len:altlen;
}

int RegExp::InnerCompile(const prechar src,int srclength,int options)
{
	int i,j;
	PREOpCode brackets[MAXDEPTH];
	// current brackets depth
	// one place reserved for surrounding 'main' brackets
	int brdepth=1;
	// compiling interior of lookbehind
	// used to apply restrictions of lookbehind
	int lookbehind=0;
	// counter of normal brackets
	int brcount=0;
	// counter of closed brackets
	// used to check correctness of backreferences
	bool closedbrackets[MAXDEPTH];
	// quoting is active
	int inquote=0;
	maxbackref=0;
#ifdef UNICODE
	UniSet *tmpclass;
#else
	rechar tmpclass[32];
	int *itmpclass=(int*)tmpclass;
#endif
	code->op=opOpenBracket;
	code->bracket.index=0;
#ifdef NAMEDBRACKETS
	MatchHash h;
	SMatch m;
#endif
	int pos=1;
	register PREOpCode op;//=code;
	brackets[0]=code;
#ifdef RE_DEBUG
#ifdef RE_NO_NEWARRAY
	resrc=static_cast<rechar*>(malloc(sizeof(rechar)*(srclength+4)));
#else
	resrc=new rechar[srclength+4];
#endif // RE_NO_NEWARRAY
	resrc[0]='(';
	resrc[1]=0;
	memcpy(resrc+1,src,srclength*sizeof(rechar));
	resrc[srclength+1]=')';
	resrc[srclength+2]=27;
	resrc[srclength+3]=0;
#endif
	havelookahead=0;

	for (i=0; i<srclength; i++)
	{
		op=code+pos;
		pos++;
#ifdef RE_DEBUG
		op->srcpos=i+1;
#endif

		if (inquote && src[i]!=backslashChar)
		{
			op->op=ignorecase?opSymbolIgnoreCase:opSymbol;
			op->symbol=ignorecase?TOLOWER(src[i]):src[i];

			if (ignorecase && TOUPPER(op->symbol)==op->symbol)op->op=opSymbol;

			continue;
		}

		if (src[i]==backslashChar)
		{
			i++;

			if (inquote && src[i]!='E')
			{
				op->op=opSymbol;
				op->symbol=backslashChar;
				op=code+pos;
				pos++;
				op->op=ignorecase?opSymbolIgnoreCase:opSymbol;
				op->symbol=ignorecase?TOLOWER(src[i]):src[i];

				if (ignorecase && TOUPPER(op->symbol)==op->symbol)op->op=opSymbol;

				continue;
			}

			op->op=opType;

			switch (src[i])
			{
				case 'Q':inquote=1; pos--; continue;
				case 'E':inquote=0; pos--; continue;
				case 'b':op->op=opWordBound; continue;
				case 'B':op->op=opNotWordBound; continue;
				case 'D':op->op=opNotType;
				case 'd':op->type=TYPE_DIGITCHAR; continue;
				case 'S':op->op=opNotType;
				case 's':op->type=TYPE_SPACECHAR; continue;
				case 'W':op->op=opNotType;
				case 'w':op->type=TYPE_WORDCHAR; continue;
				case 'U':op->op=opNotType;
				case 'u':op->type=TYPE_UPCASE; continue;
				case 'L':op->op=opNotType;
				case 'l':op->type=TYPE_LOWCASE; continue;
				case 'I':op->op=opNotType;
				case 'i':op->type=TYPE_ALPHACHAR; continue;
				case 'A':op->op=opDataStart; continue;
				case 'Z':op->op=opDataEnd; continue;
				case 'n':op->op=opSymbol; op->symbol='\n'; continue;
				case 'r':op->op=opSymbol; op->symbol='\r'; continue;
				case 't':op->op=opSymbol; op->symbol='\t'; continue;
				case 'f':op->op=opSymbol; op->symbol='\f'; continue;
				case 'e':op->op=opSymbol; op->symbol=27; continue;
				case 'O':op->op=opNoReturn; continue;
#ifdef NAMEDBRACKETS
				case 'p':
				{
					op->op=opNamedBackRef;
					i++;

					if (src[i]!='{')return SetError(errSyntax,i);

					int len=0; i++;

					while (src[i+len]!='}')len++;

					if (len>0)
					{
#ifdef RE_NO_NEWARRAY
						op->refname=static_cast<rechar*>(malloc(sizeof(rechar)*(len+1)));
#else
						op->refname=new rechar[len+1];
#endif
						memcpy(op->refname,src+i,len*sizeof(rechar));
						op->refname[len]=0;

						if (!h.Exists((char*)op->refname))
						{
							return SetError(errReferenceToUndefinedNamedBracket,i);
						}

						i+=len;
					}
					else
					{
						return SetError(errSyntax,i);
					}
				} continue;
#endif
				case 'x':
				{
					i++;

					if (i>=srclength)return SetError(errSyntax,i-1);

					if(isxdigit(src[i]))
					{
						int c=TOLOWER(src[i])-'0';

						if (c>9)c-='a'-'0'-10;

						op->op=ignorecase?opSymbolIgnoreCase:opSymbol;
						op->symbol=c;
						for(int j=1,k=i;j<4 && k+j<srclength;j++)
						{
							if(isxdigit(src[k+j]))
							{
								i++;
								c=TOLOWER(src[k+j])-'0';
								if (c>9)c-='a'-'0'-10;
								op->symbol<<=4;
								op->symbol|=c;
							}
							else
							{
								break;
							}
						}
						if (ignorecase)
						{
							op->symbol=TOLOWER(op->symbol);
							if (TOUPPER(op->symbol)==TOLOWER(op->symbol))
							{
								op->op=opSymbol;
							}
						}
					}
					else return SetError(errSyntax,i);

					continue;
				}
				default:
				{
					if (ISDIGIT(src[i]))
					{
						int save=i;
						op->op=opBackRef;
						op->refindex=GetNum(src,i); i--;

						if (op->refindex<=0 || op->refindex>brcount || !closedbrackets[op->refindex])
						{
							return SetError(errInvalidBackRef,save-1);
						}

						if (op->refindex>maxbackref)maxbackref=op->refindex;
					}
					else
					{
						if (options&OP_STRICT && ISALPHA(src[i]))
						{
							return SetError(errInvalidEscape,i-1);
						}

						op->op=ignorecase?opSymbolIgnoreCase:opSymbol;
						op->symbol=ignorecase?TOLOWER(src[i]):src[i];

						if (TOLOWER(op->symbol)==TOUPPER(op->symbol))
						{
							op->op=opSymbol;
						}
					}
				}
			}

			continue;
		}

		switch (src[i])
		{
			case '.':
			{
				if (options&OP_SINGLELINE)
				{
					op->op=opCharAnyAll;
				}
				else
				{
					op->op=opCharAny;
				}

				continue;
			}
			case '^':
			{
				if (options&OP_MULTILINE)
				{
					op->op=opLineStart;
				}
				else
				{
					op->op=opDataStart;
				}

				continue;
			}
			case '$':
			{
				if (options&OP_MULTILINE)
				{
					op->op=opLineEnd;
				}
				else
				{
					op->op=opDataEnd;
				}

				continue;
			}
			case '|':
			{
				if (brackets[brdepth-1]->op==opAlternative)
				{
					brackets[brdepth-1]->alternative.nextalt=op;
				}
				else
				{
					if (brackets[brdepth-1]->op==opOpenBracket)
					{
						brackets[brdepth-1]->bracket.nextalt=op;
					}
					else
					{
						brackets[brdepth-1]->assert.nextalt=op;
					}
				}

				if (brdepth==MAXDEPTH)return SetError(errMaxDepth,i);

				brackets[brdepth++]=op;
				op->op=opAlternative;
				continue;
			}
			case '(':
			{
				op->op=opOpenBracket;

				if (src[i+1]=='?')
				{
					i+=2;

					switch (src[i])
					{
						case ':':op->bracket.index=-1; break;
						case '=':op->op=opLookAhead; havelookahead=1; break;
						case '!':op->op=opNotLookAhead; havelookahead=1; break;
						case '<':
						{
							i++;

							if (src[i]=='=')
							{
								op->op=opLookBehind;
							}
							else if (src[i]=='!')
							{
								op->op=opNotLookBehind;
							}
							else return SetError(errSyntax,i);

							lookbehind++;
						} break;
#ifdef NAMEDBRACKETS
						case '{':
						{
							op->op=opNamedBracket;
							havenamedbrackets=1;
							int len=0;
							i++;

							while (src[i+len]!='}')len++;

							if (len>0)
							{
#ifdef RE_NO_NEWARRAY
								op->nbracket.name=static_cast<rechar*>(malloc(sizeof(rechar)*(len+1)));
#else
								op->nbracket.name=new rechar[len+1];
#endif
								memcpy(op->nbracket.name,src+i,len*sizeof(rechar));
								op->nbracket.name[len]=0;
								//h.SetItem((char*)op->nbracket.name,m);
							}
							else
							{
								op->op=opOpenBracket;
								op->bracket.index=-1;
							}

							i+=len;
						} break;
#endif
						default:
						{
							return SetError(errSyntax,i);
						}
					}
				}
				else
				{
					brcount++;
					closedbrackets[brcount]=false;
					op->bracket.index=brcount;
				}

				brackets[brdepth]=op;
				brdepth++;
				continue;
			}
			case ')':
			{
				op->op=opClosingBracket;
				brdepth--;

				while (brackets[brdepth]->op==opAlternative)
				{
					brackets[brdepth]->alternative.endindex=op;
					brdepth--;
				}

				switch (brackets[brdepth]->op)
				{
					case opOpenBracket:
					{
						op->bracket.pairindex=brackets[brdepth];
						brackets[brdepth]->bracket.pairindex=op;
						op->bracket.index=brackets[brdepth]->bracket.index;

						if (op->bracket.index!=-1)
						{
							closedbrackets[op->bracket.index]=true;
						}

						break;
					}
#ifdef NAMEDBRACKETS
					case opNamedBracket:
					{
						op->nbracket.pairindex=brackets[brdepth];
						brackets[brdepth]->nbracket.pairindex=op;
						op->nbracket.name=brackets[brdepth]->nbracket.name;
						h.SetItem((char*)op->nbracket.name,m);
						break;
					}
#endif
					case opLookBehind:
					case opNotLookBehind:
					{
						lookbehind--;
						int l=CalcPatternLength(brackets[brdepth]->next,op->prev);

						if (l==-1)return SetError(errVariableLengthLookBehind,i);

						brackets[brdepth]->assert.length=l;
					}// there is no break and this is correct!
					case opLookAhead:
					case opNotLookAhead:
					{
						op->assert.pairindex=brackets[brdepth];
						brackets[brdepth]->assert.pairindex=op;
						break;
					}
				}

				continue;
			}
			case '[':
			{
				i++;
				int negative=0;

				if (src[i]=='^')
				{
					negative=1;
					i++;
				}

				int lastchar=0;
				int classsize=0;
				op->op=opSymbolClass;
				//op->symbolclass=new rechar[32];
				//memset(op->symbolclass,0,32);
#ifdef UNICODE
				op->symbolclass=new UniSet();
				tmpclass=op->symbolclass;
#else

				for (j=0; j<8; j++)itmpclass[j]=0;

#endif
				int classindex=0;

				for (; src[i]!=']'; i++)
				{
					if (src[i]==backslashChar)
					{
						i++;
						int isnottype=0;
						int type=0;
						lastchar=0;

						switch (src[i])
						{
							case 'D':isnottype=1;
							case 'd':type=TYPE_DIGITCHAR; classindex=0; break;
							case 'W':isnottype=1;
							case 'w':type=TYPE_WORDCHAR; classindex=64; break;
							case 'S':isnottype=1;
							case 's':type=TYPE_SPACECHAR; classindex=32; break;
							case 'L':isnottype=1;
							case 'l':type=TYPE_LOWCASE; classindex=96; break;
							case 'U':isnottype=1;
							case 'u':type=TYPE_UPCASE; classindex=128; break;
							case 'I':isnottype=1;
							case 'i':type=TYPE_ALPHACHAR; classindex=160; break;
							case 'n':lastchar='\n'; break;
							case 'r':lastchar='\r'; break;
							case 't':lastchar='\t'; break;
							case 'f':lastchar='\f'; break;
							case 'e':lastchar=27; break;
							case 'x':
							{
								i++;

								if (i>=srclength)return SetError(errSyntax,i-1);

								if (isxdigit(src[i]))
								{
									int c=TOLOWER(src[i])-'0';

									if (c>9)c-='a'-'0'-10;

									lastchar=c;

									for(int j=1,k=i;j<4 && k+j<srclength;j++)
									{
										if (isxdigit(src[k+j]))
										{
											i++;
											c=TOLOWER(src[k+j])-'0';

											if (c>9)c-='a'-'0'-10;

											lastchar<<=4;
											lastchar|=c;
										}
										else
										{
											break;
										}
									}
									dpf(("Last char=%c(%02x)\n",lastchar,lastchar));
								}
								else return SetError(errSyntax,i);

								break;
							}
							default:
							{
								if (options&OP_STRICT && ISALPHA(src[i]))
								{
									return SetError(errInvalidEscape,i-1);
								}

								lastchar=src[i];
							}
						}

						if (type)
						{
#ifdef UNICODE

							if (isnottype)
							{
								tmpclass->nottypes|=type;
							}
							else
							{
								tmpclass->types|=type;
							}

#else
							isnottype=isnottype?0xffffffff:0;
							int *b=(int*)(charbits+classindex);

							for (j=0; j<8; j++)
							{
								itmpclass[j]|=b[j]^isnottype;
							}

#endif
							classsize=257;
							//for(int j=0;j<32;j++)op->symbolclass[j]|=charbits[classindex+j]^isnottype;
							//classsize+=charsizes[classindex>>5];
							//int setbit;
							/*for(int j=0;j<256;j++)
							{
							  setbit=(chartypes[j]^isnottype)&type;
							  if(setbit)
							  {
							    if(ignorecase)
							    {
							      SetBit(op->symbolclass,lc[j]);
							      SetBit(op->symbolclass,uc[j]);
							    }else
							    {
							      SetBit(op->symbolclass,j);
							    }
							    classsize++;
							  }
							}*/
						}
						else
						{
							if (options&OP_IGNORECASE)
							{
								SetBit(tmpclass,TOLOWER(lastchar));
								SetBit(tmpclass,TOUPPER(lastchar));
							}
							else
							{
								SetBit(tmpclass,lastchar);
							}

							classsize++;
						}

						continue;
					}

					if (src[i]=='-')
					{
						if (lastchar && src[i+1]!=']')
						{
							int to=src[i+1];

							if (to==backslashChar)
							{
								to=src[i+2];

								if (to=='x')
								{
									i+=2;
									to=TOLOWER(src[i+1]);

									if(isxdigit(to))
									{
										to-='0';

										if (to>9)to-='a'-'0'-10;

										for(int j=1,k=(i+1);j<4 && k+j<srclength;j++)
										{
											int c=TOLOWER(src[k+j]);
											if(isxdigit(c))
											{
												i++;
												c-='0';

												if (c>9)c-='a'-'0'-10;

												to<<=4;
												to|=c;
											}
											else
											{
												break;
											}
										}
									}
									else return SetError(errSyntax,i);
								}
								else
								{
									SetBit(tmpclass,'-');
									classsize++;
									continue;
								}
							}

							i++;
							dpf(("from %d to %d\n",lastchar,to));

							for (j=lastchar; j<=to; j++)
							{
								if (ignorecase)
								{
									SetBit(tmpclass,TOLOWER(j));
									SetBit(tmpclass,TOUPPER(j));
								}
								else
								{
									SetBit(tmpclass,j);
								}

								classsize++;
							}

							continue;
						}
					}

					lastchar=src[i];

					if (ignorecase)
					{
						SetBit(tmpclass,TOLOWER(lastchar));
						SetBit(tmpclass,TOUPPER(lastchar));
					}
					else
					{
						SetBit(tmpclass,lastchar);
					}

					classsize++;
				}

				if (negative && classsize>1)
				{
#ifdef UNICODE
					tmpclass->negative=negative;
#else

					for (int jj=0; jj<8; jj++)itmpclass[jj]^=0xffffffff;

#endif
					//for(int j=0;j<32;j++)op->symbolclass[j]^=0xff;
				}

				if (classsize==1)
				{
#ifdef UNICODE
					delete op->symbolclass;
					op->symbolclass=0;
					tmpclass=0;
#endif
					op->op=negative?opNotSymbol:opSymbol;

					if (ignorecase)
					{
						op->op+=2;
						op->symbol=TOLOWER(lastchar);
					}
					else
					{
						op->symbol=lastchar;
					}
				}

#ifdef UNICODE

				if (tmpclass)tmpclass->negative=negative;

#else
				else if (classsize==256 && !negative)
				{
					op->op=options&OP_SINGLELINE?opCharAnyAll:opCharAny;
				}
				else
				{
#ifdef RE_NO_NEWARRAY
					op->symbolclass=static_cast<rechar*>(malloc(sizeof(rechar)*32));
#else
				op->symbolclass=new rechar[32];
#endif

					for (j=0; j<8; j++)((int*)op->symbolclass)[j]=itmpclass[j];
				}

#endif
				continue;
			}
			case '+':
			case '*':
			case '?':
			case '{':
			{
				int min=0,max=0;

				switch (src[i])
				{
					case '+':min=1; max=-2; break;
					case '*':min=0; max=-2; break;
					case '?':
					{
						//if(src[i+1]=='?') return SetError(errInvalidQuantifiersCombination,i);
						min=0; max=1;
						break;
					}
					case '{':
					{
						i++;
						int save=i;
						min=GetNum(src,i);
						max=min;

						if (min<0)return SetError(errInvalidRange,save);

//            i++;
						if (src[i]==',')
						{
							if (src[i+1]=='}')
							{
								i++;
								max=-2;
							}
							else
							{
								i++;
								max=GetNum(src,i);

//                i++;
								if (max<min)return SetError(errInvalidRange,save);
							}
						}

						if (src[i]!='}')return SetError(errInvalidRange,save);
					}
				}

				pos--;
				op=code+pos-1;

				if (min==1 && max==1)continue;

				op->range.min=min;
				op->range.max=max;

				switch (op->op)
				{
					case opLineStart:
					case opLineEnd:
					case opDataStart:
					case opDataEnd:
					case opWordBound:
					case opNotWordBound:
					{
						return SetError(errInvalidQuantifiersCombination,i);
//            op->range.op=op->op;
//            op->op=opRange;
//            continue;
					}
					case opCharAny:
					case opCharAnyAll:
					{
						op->range.op=op->op;
						op->op=opAnyRange;
						break;
					}
					case opType:
					{
						op->op=opTypeRange;
						break;
					}
					case opNotType:
					{
						op->op=opNotTypeRange;
						break;
					}
					case opSymbolIgnoreCase:
					case opSymbol:
					{
						op->op=opSymbolRange;
						break;
					}
					case opNotSymbol:
					case opNotSymbolIgnoreCase:
					{
						op->op=opNotSymbolRange;
						break;
					}
					case opSymbolClass:
					{
						op->op=opClassRange;
						break;
					}
					case opBackRef:
					{
						op->op=opBackRefRange;
						break;
					}
#ifdef NAMEDBRACKETS
					case opNamedBackRef:
					{
						op->op=opNamedRefRange;
					} break;
#endif
					case opClosingBracket:
					{
						op=op->bracket.pairindex;

						if (op->op!=opOpenBracket)return SetError(errInvalidQuantifiersCombination,i);

						op->range.min=min;
						op->range.max=max;
						op->op=opBracketRange;
						break;
					}
					default:
					{
						dpf(("OP=%d\n",op->op));
						return SetError(errInvalidQuantifiersCombination,i);
					}
				}//switch(code.op)

				if (src[i+1]=='?')
				{
					op->op++;
					i++;
				}

				continue;
			}// case +*?{
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			{
				if (options&OP_XTENDEDSYNTAX)
				{
					pos--;
					continue;
				}
			}
#ifdef RELIB
			case '%':
			{
				i++;
				int len=0;

				while (src[i+len]!='%')len++;

				op->op=opLibCall;
#ifdef RE_NO_NEWARRAY
				op->rename=static_cast<rechar*>(malloc(sizeof(rechar)*(len+1)));
#else
				op->rename=new rechar[len+1];
#endif
				memcpy(op->rename,src+i,len*sizeof(rechar));
				op->rename[len]=0;
				i+=len;
				continue;
			}
#endif
			default:
			{
				op->op=options&OP_IGNORECASE?opSymbolIgnoreCase:opSymbol;

				if (ignorecase)
				{
					op->symbol=TOLOWER(src[i]);
				}
				else
				{
					op->symbol=src[i];
				}
			}
		}//switch(src[i])
	}//for()

	op=code+pos;
	pos++;
	brdepth--;

	while (brdepth>=0 && brackets[brdepth]->op==opAlternative)
	{
		brackets[brdepth]->alternative.endindex=op;
		brdepth--;
	}

	op->op=opClosingBracket;
	op->bracket.pairindex=code;
	code->bracket.pairindex=op;
#ifdef RE_DEBUG
	op->srcpos=i;
#endif
	op=code+pos;
	//pos++;
	op->op=opRegExpEnd;
#ifdef RE_DEBUG
	op->srcpos=i+1;
#endif
	return 1;
}

inline void RegExp::PushState()
{
	stackcount++;
#ifdef RELIB
	stackusage++;
#endif

	if (stackcount==STACK_PAGE_SIZE)
	{
		if (lastpage->next)
		{
			lastpage=lastpage->next;
			stack=lastpage->stack;
		}
		else
		{
			lastpage->next=new StateStackPage;
			lastpage->next->prev=lastpage;
			lastpage=lastpage->next;
			lastpage->next=nullptr;
#ifdef RE_NO_NEWARRAY
			lastpage->stack=static_cast<StateStackItem*>
			                (CreateArray(sizeof(StateStackItem), STACK_PAGE_SIZE,
			                             StateStackItem::OnCreate));
#else
			lastpage->stack=new StateStackItem[STACK_PAGE_SIZE];
#endif // RE_NO_NEWARRAY
			stack=lastpage->stack;
		}

		stackcount=0;
	}

	st=&stack[stackcount];
}
inline int RegExp::PopState()
{
	stackcount--;
#ifdef RELIB
	stackusage--;

	if (stackusage<0)return 0;

#endif

	if (stackcount<0)
	{
		if (lastpage->prev==nullptr)return 0;

		lastpage=lastpage->prev;
		stack=lastpage->stack;
		stackcount=STACK_PAGE_SIZE-1;
	}

	st=&stack[stackcount];
	return 1;
}


inline StateStackItem *RegExp::GetState()
{
	int tempcount=stackcount;
#ifdef RELIB

	if (stackusage==0)return 0;

#endif
	StateStackPage* temppage=lastpage;
	StateStackItem* tempstack=lastpage->stack;
	tempcount--;

	if (tempcount<0)
	{
		if (temppage->prev==nullptr)return 0;

		temppage=temppage->prev;
		tempstack=temppage->stack;
		tempcount=STACK_PAGE_SIZE-1;
	}

	return &tempstack[tempcount];
}

inline StateStackItem *RegExp::FindStateByPos(PREOpCode pos,int op)
{
#ifdef RELIB
	int tempusage=stackusage;
#endif
	int tempcount=stackcount;
	StateStackPage* temppage=lastpage;
	StateStackItem* tempstack=lastpage->stack;

	do
	{
		tempcount--;
#ifdef RELIB
		tempusage--;

		if (tempusage<0)return 0;

#endif

		if (tempcount<0)
		{
			if (temppage->prev==nullptr)return 0;

			temppage=temppage->prev;
			tempstack=temppage->stack;
			tempcount=STACK_PAGE_SIZE-1;
		}
	}
	while (tempstack[tempcount].pos!=pos || tempstack[tempcount].op!=op);

	return &tempstack[tempcount];
}


inline int RegExp::StrCmp(const prechar& str,const prechar _st,const prechar ed)
{
	const prechar save=str;

	if (ignorecase)
	{
		while (_st<ed)
		{
			if (TOLOWER(*str)!=TOLOWER(*_st)) {str=save; return 0;}

			str++;
			_st++;
		}
	}
	else
	{
		while (_st<ed)
		{
			if (*str!=*_st) {str=save; return 0;}

			str++;
			_st++;
		}
	}

	return 1;
}

#define OP (*op)


#define MINSKIP(cmp) \
	{ int jj; \
		switch(op->next->op) \
		{ \
			case opSymbol: \
			{ \
				jj=op->next->symbol; \
				if(*str!=jj) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if(str[1]!=jj)break;\
					} \
				break; \
			} \
			case opNotSymbol: \
			{ \
				jj=op->next->symbol; \
				if(*str==jj) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if(str[1]==jj)break;\
					} \
				break; \
			} \
			case opSymbolIgnoreCase: \
			{ \
				jj=op->next->symbol; \
				if(TOLOWER(*str)!=jj) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if(TOLOWER(str[1])!=jj)break;\
					} \
				break; \
			} \
			case opNotSymbolIgnoreCase: \
			{ \
				jj=op->next->symbol; \
				if(TOLOWER(*str)==jj) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if(TOLOWER(str[1])==jj)break;\
					} \
				break; \
			} \
			case opType: \
			{ \
				jj=op->next->type; \
				if(!(ISTYPE(*str,jj))) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if(!(ISTYPE(str[1],jj)))break;\
					} \
				break; \
			} \
			case opNotType: \
			{ \
				jj=op->next->type; \
				if((ISTYPE(*str,jj))) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if((ISTYPE(str[1],jj)))break;\
					} \
				break; \
			} \
			case opSymbolClass: \
			{ \
				cl=op->next->symbolclass; \
				if(!GetBit(cl,*str)) \
					while(str<strend && cmp && st->max--!=0)\
					{\
						str++;\
						if(!GetBit(cl,str[1]))break;\
					} \
				break; \
			} \
		} \
	}

#ifdef RELIB
static void KillMatchList(MatchList *ml)
{
	for (int i=0; i<ml->Count(); i++)
	{
		KillMatchList((*ml)[i].sublist);
		(*ml)[i].sublist=nullptr;
	}

	ml->Clean();
}

#endif


int RegExp::InnerMatch(const prechar str,const prechar strend,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                       ,PMatchHash hmatch
#endif
                      )
{
//  register prechar str=start;
	int i,j;
	int minimizing;
	PREOpCode op,tmp=nullptr;
	PMatch m;
#ifdef UNICODE
	UniSet *cl;
#else
	prechar cl;
#endif
#ifdef RELIB
	SMatchListItem ml;
#endif
	int inrangebracket=0;

	if (errorcode==errNotCompiled)return 0;

	if (matchcount<maxbackref)return SetError(errNotEnoughMatches,maxbackref);

#ifdef NAMEDBRACKETS

	if (havenamedbrackets && !hmatch)return SetError(errNoStorageForNB,0);

#endif
#ifdef RELIB

	if (reclevel<=1)
	{
#endif
		stackcount=0;
		lastpage=firstpage;
		stack=lastpage->stack;
		st=&stack[0];
#ifdef RELIB
	}

#endif
	StateStackItem *ps;
	errorcode=errNone;

	/*for(i=0;i<matchcount;i++)
	{
	  match[i].start=-1;
	  match[i].end=-1;
	}*/
	if (bracketscount<matchcount)matchcount=bracketscount;

	memset(match,-1,sizeof(*match)*matchcount);

	for (op=code; op; op=op->next)
	{
		//dpf(("op:%s,\tpos:%d,\tstr:%d\n",ops[OP.op],pos,str-start));
		dpf(("=================\n"));
		dpf(("S:%s\n%*s\n",start,str-start+3,"^"));
		dpf(("R:%s\n%*s\n",resrc,OP.srcpos+3,"^"));

		if (str<=strend)
			switch (OP.op)
			{
				case opLineStart:
				{
					if (str==start || str[-1]==0x0d || str[-1]==0x0a)continue;

					break;
				}
				case opLineEnd:
				{
					if (str==strend)continue;

					if (str[0]==0x0d || str[0]==0x0a)
					{
						if (str[0]==0x0d)str++;

						if (str[0]==0x0a)str++;

						continue;
					}

					break;
				}
				case opDataStart:
				{
					if (str==start)continue;

					break;
				}
				case opDataEnd:
				{
					if (str==strend)continue;

					break;
				}
				case opWordBound:
				{
					if ((str==start && ISWORD(*str))||
					        (!(ISWORD(str[-1])) && ISWORD(*str)) ||
					        (!(ISWORD(*str)) && ISWORD(str[-1])) ||
					        (str==strend && ISWORD(str[-1])))continue;

					break;
				}
				case opNotWordBound:
				{
					if (!((str==start && ISWORD(*str))||
					        (!(ISWORD(str[-1])) && ISWORD(*str)) ||
					        (!(ISWORD(*str)) && ISWORD(str[-1])) ||
					        (str==strend && ISWORD(str[-1]))))continue;

					break;
				}
				case opType:
				{
					if (ISTYPE(*str,OP.type))
					{
						str++;
						continue;
					}

					break;
				}
				case opNotType:
				{
					if (!(ISTYPE(*str,OP.type)))
					{
						str++;
						continue;
					}

					break;
				}
				case opCharAny:
				{
					if (*str!=0x0d && *str!=0x0a)
					{
						str++;
						continue;
					}

					break;
				}
				case opCharAnyAll:
				{
					str++;
					continue;
				}
				case opSymbol:
				{
					if (*str==OP.symbol)
					{
						str++;
						continue;
					}

					break;
				}
				case opNotSymbol:
				{
					if (*str!=OP.symbol)
					{
						str++;
						continue;
					}

					break;
				}
				case opSymbolIgnoreCase:
				{
					if (TOLOWER(*str)==OP.symbol)
					{
						str++;
						continue;
					}

					break;
				}
				case opNotSymbolIgnoreCase:
				{
					if (TOLOWER(*str)!=OP.symbol)
					{
						str++;
						continue;
					}

					break;
				}
				case opSymbolClass:
				{
					if (GetBit(OP.symbolclass,*str))
					{
						str++;
						continue;
					}

					break;
				}
				case opOpenBracket:
				{
					if (OP.bracket.index>=0 && OP.bracket.index<matchcount)
					{
						if (inrangebracket)
						{
							st->op=opOpenBracket;
							st->pos=op;
							st->min=match[OP.bracket.index].start;
							st->max=match[OP.bracket.index].end;
							PushState();
						}

						match[OP.bracket.index].start=(int)(str-start);
					}

					if (OP.bracket.nextalt)
					{
						st->op=opAlternative;
						st->pos=OP.bracket.nextalt;
						st->savestr=str;
						PushState();
					}

					continue;
				}
#ifdef NAMEDBRACKETS
				case opNamedBracket:
				{
					if (hmatch)
					{
						PMatch m2;

						if (!hmatch->Exists((char*)OP.nbracket.name))
						{
							tag_Match sm;
							sm.start=-1;
							sm.end=-1;
							m2=hmatch->SetItem((char*)OP.nbracket.name,sm);
						}
						else
						{
							m2=hmatch->GetPtr((char*)OP.nbracket.name);
						}

						if (inrangebracket)
						{
							st->op=opNamedBracket;
							st->pos=op;
							st->min=m2->start;
							st->max=m2->end;
							PushState();
						}

						m2->start=(int)(str-start);
					}

					if (OP.bracket.nextalt)
					{
						st->op=opAlternative;
						st->pos=OP.bracket.nextalt;
						st->savestr=str;
						PushState();
					}

					continue;
				}
#endif
				case opClosingBracket:
				{
					switch (OP.bracket.pairindex->op)
					{
						case opOpenBracket:
						{
							if (OP.bracket.index>=0 && OP.bracket.index<matchcount)
							{
								match[OP.bracket.index].end=(int)(str-start);

								if (brhandler)
								{
									if (
									    !brhandler
									    (
									        brhdata,
									        bhMatch,
									        OP.bracket.index,
									        match[OP.bracket.index].start,
									        match[OP.bracket.index].end
									    )
									)
									{
										return -1;
									}
								}
							}

							continue;
						}
#ifdef NAMEDBRACKETS
						case opNamedBracket:
						{
							if (hmatch)
							{
								PMatch m2=hmatch->GetPtr((char*)OP.nbracket.name);
								m2->end=(int)(str-start);
							}

							continue;
						}
#endif
						case opBracketRange:
						{
							ps=FindStateByPos(OP.bracket.pairindex,opBracketRange);
							*st=*ps;

							if (str==st->startstr)
							{
								if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
								{
									match[OP.range.bracket.index].end=(int)(str-start);

									if (brhandler)
									{
										if (
										    !brhandler
										    (
										        brhdata,
										        bhMatch,
										        OP.range.bracket.index,
										        match[OP.range.bracket.index].start,
										        match[OP.range.bracket.index].end
										    )
										)
										{
											return -1;
										}
									}
								}

								inrangebracket--;
								continue;
							}

							if (st->min>0)st->min--;

							if (st->min)
							{
								st->max--;
								st->startstr=str;
								st->savestr=str;
								op=st->pos;
								PushState();

								if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
								{
									match[OP.range.bracket.index].start=(int)(str-start);
									st->op=opOpenBracket;
									st->pos=op;
									st->min=match[OP.range.bracket.index].start;
									st->max=match[OP.range.bracket.index].end;
									PushState();
								}

								if (OP.range.bracket.nextalt)
								{
									st->op=opAlternative;
									st->pos=OP.range.bracket.nextalt;
									st->savestr=str;
									PushState();
								}

								continue;
							}

							st->max--;

							if (st->max==0)
							{
								if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
								{
									match[OP.range.bracket.index].end=(int)(str-start);

									if (brhandler)
									{
										if (
										    !brhandler
										    (
										        brhdata,
										        bhMatch,
										        OP.range.bracket.index,
										        match[OP.range.bracket.index].start,
										        match[OP.range.bracket.index].end
										    )
										)
										{
											return -1;
										}
									}
								}

								inrangebracket--;
								continue;
							}

							if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
							{
								match[OP.range.bracket.index].end=(int)(str-start);

								if (brhandler)
								{
									if (
									    !brhandler
									    (
									        brhdata,
									        bhMatch,
									        OP.range.bracket.index,
									        match[OP.range.bracket.index].start,
									        match[OP.range.bracket.index].end
									    )
									)
									{
										return -1;
									}
								}

								tmp=op;
							}

							st->startstr=str;
							st->savestr=str;
							op=st->pos;
							PushState();

							if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
							{
								st->op=opOpenBracket;
								st->pos=tmp;
								st->min=match[OP.range.bracket.index].start;
								st->max=match[OP.range.bracket.index].end;
								PushState();
								match[OP.range.bracket.index].start=(int)(str-start);
							}

							if (OP.range.bracket.nextalt)
							{
								st->op=opAlternative;
								st->pos=OP.range.bracket.nextalt;
								st->savestr=str;
								PushState();
							}

							continue;
						}
						case opBracketMinRange:
						{
							ps=FindStateByPos(OP.bracket.pairindex,opBracketMinRange);
							*st=*ps;

							if (st->min>0)st->min--;

							if (st->min)
							{
								//st->min--;
								st->max--;
								st->startstr=str;
								st->savestr=str;
								op=st->pos;
								PushState();

								if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
								{
									if (brhandler)
									{
										if (
										    !brhandler
										    (
										        brhdata,
										        bhMatch,
										        OP.range.bracket.index,
										        match[OP.range.bracket.index].start,
										        (int)(str-start)
										    )
										)
										{
											return -1;
										}
									}

									match[OP.range.bracket.index].start=(int)(str-start);
									st->op=opOpenBracket;
									st->pos=op;
									st->min=match[OP.range.bracket.index].start;
									st->max=match[OP.range.bracket.index].end;
									PushState();
								}

								if (OP.range.bracket.nextalt)
								{
									st->op=opAlternative;
									st->pos=OP.range.bracket.nextalt;
									st->savestr=str;
									PushState();
								}

								continue;
							}

							if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
							{
								match[OP.range.bracket.index].end=(int)(str-start);

								if (brhandler)
								{
									if (
									    !brhandler
									    (
									        brhdata,
									        bhMatch,
									        OP.range.bracket.index,
									        match[OP.range.bracket.index].start,
									        match[OP.range.bracket.index].end
									    )
									)
									{
										return -1;
									}
								}
							}

							st->max--;
							inrangebracket--;

							if (st->max==0)continue;

							st->forward=str>ps->startstr?1:0;
							st->startstr=str;
							st->savestr=str;
							PushState();

							if (OP.range.bracket.nextalt)
							{
								st->op=opAlternative;
								st->pos=OP.range.bracket.nextalt;
								st->savestr=str;
								PushState();
							}

							continue;
						}
						case opLookAhead:
						{
							tmp=OP.bracket.pairindex;

							do
							{
								PopState();
							}
							while (st->pos!=tmp || st->op!=opLookAhead);

							str=st->savestr;
							continue;
						}
						case opNotLookAhead:
						{
							do
							{
								PopState();
							}
							while (st->op!=opNotLookAhead);

							str=st->savestr;
							break;
						}
						case opLookBehind:
						{
							continue;
						}
						case opNotLookBehind:
						{
							ps=GetState();
							ps->forward=0;
							break;
						}
					}//switch(code[pairindex].op)

					break;
				}//case opClosingBracket
				case opAlternative:
				{
					op=OP.alternative.endindex->prev;
					continue;
				}
#ifdef NAMEDBRACKETS
				case opNamedBackRef:
#endif
				case opBackRef:
				{
#ifdef NAMEDBRACKETS

					if (OP.op==opNamedBackRef)
					{
						if (!hmatch || !hmatch->Exists((char*)OP.refname))break;

						m=hmatch->GetPtr((char*)OP.refname);
					}
					else
					{
						m=&match[OP.refindex];
					}

#else
					m=&match[OP.refindex];
#endif

					if (m->start==-1 || m->end==-1)break;

					if (ignorecase)
					{
						j=m->end;

						for (i=m->start; i<j; i++,str++)
						{
							if (TOLOWER(start[i])!=TOLOWER(*str))break;

							if (str>strend)break;
						}

						if (i<j)break;
					}
					else
					{
						j=m->end;

						for (i=m->start; i<j; i++,str++)
						{
							if (start[i]!=*str)break;

							if (str>strend)break;
						}

						if (i<j)break;
					}

					continue;
				}
				case opAnyRange:
				case opAnyMinRange:
				{
					st->op=OP.op;
					minimizing=OP.op==opAnyMinRange;
					j=OP.range.min;
					st->max=OP.range.max-j;

					if (OP.range.op==opCharAny)
					{
						for (i=0; i<j; i++,str++)
						{
							if (str>strend || *str==0x0d || *str==0x0a)break;
						}

						if (i<j)
						{
							break;
						}

						st->startstr=str;

						if (!minimizing)
						{
							while (str<strend && *str!=0x0d && *str!=0x0a && st->max--!=0)str++;
						}
						else
						{
							MINSKIP(*str!=0x0d && *str!=0x0a);

							if (st->max==-1)break;
						}
					}
					else
					{
						//opCharAnyAll:
						str+=j;

						if (str>strend)break;

						st->startstr=str;

						if (!minimizing)
						{
							if (st->max>=0)
							{
								if (str+st->max<strend)
								{
									str+=st->max;
									st->max=0;
								}
								else
								{
									st->max-=(int)(strend-str);
									str=strend;
								}
							}
							else
							{
								str=strend;
							}
						}
						else
						{
							MINSKIP(1);

							if (st->max==-1)break;
						}
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
				case opSymbolRange:
				case opSymbolMinRange:
				{
					st->op=OP.op;
					minimizing=OP.op==opSymbolMinRange;
					j=OP.range.min;
					st->max=OP.range.max-j;

					if (ignorecase)
					{
						for (i=0; i<j; i++,str++)
						{
							if (str>strend || TOLOWER(*str)!=OP.range.symbol)break;
						}

						if (i<j)break;

						st->startstr=str;

						if (!minimizing)
						{
							while (str<strend && TOLOWER(*str)==OP.range.symbol && st->max--!=0)str++;
						}
						else
						{
							MINSKIP(TOLOWER(*str)==OP.range.symbol);

							if (st->max==-1)break;
						}
					}
					else
					{
						for (i=0; i<j; i++,str++)
						{
							if (str>strend || *str!=OP.range.symbol)break;
						}

						if (i<j)break;

						st->startstr=str;

						if (!minimizing)
						{
							while (str<strend && *str==OP.range.symbol && st->max--!=0)str++;
						}
						else
						{
							MINSKIP(*str==OP.range.symbol);

							if (st->max==-1)break;
						}
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
				case opNotSymbolRange:
				case opNotSymbolMinRange:
				{
					st->op=OP.op;
					minimizing=OP.op==opNotSymbolMinRange;
					j=OP.range.min;
					st->max=OP.range.max-j;

					if (ignorecase)
					{
						for (i=0; i<j; i++,str++)
						{
							if (str>strend || TOLOWER(*str)==OP.range.symbol)break;
						}

						if (i<j)break;

						st->startstr=str;

						if (!minimizing)
						{
							while (str<strend && TOLOWER(*str)!=OP.range.symbol && st->max--!=0)str++;
						}
						else
						{
							MINSKIP(TOLOWER(*str)!=OP.range.symbol);

							if (st->max==-1)break;
						}
					}
					else
					{
						for (i=0; i<j; i++,str++)
						{
							if (str>strend || *str==OP.range.symbol)break;
						}

						if (i<j)break;

						st->startstr=str;

						if (!minimizing)
						{
							while (str<strend && *str!=OP.range.symbol && st->max--!=0)str++;
						}
						else
						{
							MINSKIP(*str!=OP.range.symbol);

							if (st->max==-1)break;
						}
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
				case opClassRange:
				case opClassMinRange:
				{
					st->op=OP.op;
					minimizing=OP.op==opClassMinRange;
					j=OP.range.min;
					st->max=OP.range.max-j;

					for (i=0; i<j; i++,str++)
					{
						if (str>strend || !GetBit(OP.range.symbolclass,*str))break;
					}

					if (i<j)break;

					st->startstr=str;

					if (!minimizing)
					{
						while (str<strend && GetBit(OP.range.symbolclass,*str) && st->max--!=0)str++;
					}
					else
					{
						MINSKIP(GetBit(OP.range.symbolclass,*str));

						if (st->max==-1)break;
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
				case opTypeRange:
				case opTypeMinRange:
				{
					st->op=OP.op;
					minimizing=OP.op==opTypeMinRange;
					j=OP.range.min;
					st->max=OP.range.max-j;

					for (i=0; i<j; i++,str++)
					{
						if (str>strend || (ISTYPE(*str,OP.range.type))==0)break;
					}

					if (i<j)break;

					st->startstr=str;

					if (!minimizing)
					{
						while (str<strend && (ISTYPE(*str,OP.range.type)) && st->max--!=0)str++;
					}
					else
					{
						MINSKIP((ISTYPE(*str,OP.range.type)));

						if (st->max==-1)break;
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
				case opNotTypeRange:
				case opNotTypeMinRange:
				{
					st->op=OP.op;
					minimizing=OP.op==opNotTypeMinRange;
					j=OP.range.min;
					st->max=OP.range.max-j;

					for (i=0; i<j; i++,str++)
					{
						if (str>strend || (ISTYPE(*str,OP.range.type))!=0)break;
					}

					if (i<j)break;

					st->startstr=str;

					if (!minimizing)
					{
						while (str<strend && (ISTYPE(*str,OP.range.type))==0 && st->max--!=0)str++;
					}
					else
					{
						MINSKIP((ISTYPE(*str,OP.range.type))==0);

						if (st->max==-1)break;
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
#ifdef NAMEDBRACKETS
				case opNamedRefRange:
				case opNamedRefMinRange:
#endif
				case opBackRefRange:
				case opBackRefMinRange:
				{
					st->op=OP.op;
#ifdef NAMEDBRACKETS
					minimizing=OP.op==opBackRefMinRange || OP.op==opNamedRefMinRange;
#else
					minimizing=OP.op==opBackRefMinRange;
#endif
					j=OP.range.min;
					st->max=OP.range.max-j;
#ifdef NAMEDBRACKETS

					if (OP.op==opBackRefRange || OP.op==opBackRefMinRange)
					{
						m=&match[OP.range.refindex];
					}
					else
					{
						m=hmatch->GetPtr((char*)OP.range.refname);
					}

#else
					m=&match[OP.range.refindex];
#endif

					if (!m)break;

					if (m->start==-1 || m->end==-1)
					{
						if (j==0)continue;
						else break;
					}

					for (i=0; i<j; i++)
					{
						if (str>strend || StrCmp(str,start+m->start,start+m->end)==0)break;
					}

					if (i<j)break;

					st->startstr=str;

					if (!minimizing)
					{
						while (str<strend && StrCmp(str,start+m->start,start+m->end) && st->max--!=0);
					}
					else
					{
						MINSKIP(StrCmp(str,start+m->start,start+m->end));

						if (st->max==-1)break;
					}

					if (OP.range.max==j)continue;

					st->savestr=str;
					st->pos=op;
					PushState();
					continue;
				}
				case opBracketRange:
				case opBracketMinRange:
				{
					if (inrangebracket && OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
					{
						st->op=opOpenBracket;
						st->pos=OP.range.bracket.pairindex;
						st->min=match[OP.range.bracket.index].start;
						st->max=match[OP.range.bracket.index].end;
						PushState();
					}

					st->op=OP.op;
					st->pos=op;
					st->min=OP.range.min;
					st->max=OP.range.max;
					st->startstr=str;
					st->savestr=str;
					st->forward=1;
					PushState();

					if (OP.range.nextalt)
					{
						st->op=opAlternative;
						st->pos=OP.range.bracket.nextalt;
						st->savestr=str;
						PushState();
					}

					if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
					{
						match[OP.range.bracket.index].start=
						    /*match[OP.range.bracket.index].end=*/(int)(str-start);
					}

					if (OP.op==opBracketMinRange && OP.range.min==0)
					{
						op=OP.range.bracket.pairindex;
					}
					else
					{
						inrangebracket++;
					}

					continue;
				}
				case opLookAhead:
				case opNotLookAhead:
				{
					st->op=OP.op;
					st->savestr=str;
					st->pos=op;
					st->forward=1;
					PushState();

					if (OP.assert.nextalt)
					{
						st->op=opAlternative;
						st->pos=OP.assert.nextalt;
						st->savestr=str;
						PushState();
					}

					continue;
				}
				case opLookBehind:
				case opNotLookBehind:
				{
					if (str-OP.assert.length<start)
					{
						if (OP.op==opLookBehind)break;

						op=OP.assert.pairindex;
						continue;
					}

					st->op=OP.op;
					st->savestr=str;
					st->pos=op;
					st->forward=1;
					str-=OP.assert.length;
					PushState();

					if (OP.assert.nextalt)
					{
						st->op=opAlternative;
						st->pos=OP.assert.nextalt;
						st->savestr=str;
						PushState();
					}

					continue;
				}
				case opNoReturn:
				{
					st->op=opNoReturn;
					PushState();
					continue;
				}
#ifdef RELIB
				case opLibCall:
				{
					if (!relib->Exists((char*)OP.rename))return 0;

					/*        int ok=1;
					        PMatchList curlist=matchlist;
					        while(curlist)
					        {
					          if(curlist->Count()==0)
					          {
					            curlist=curlist->parent;
					            continue;
					          }
					          int k=curlist->Count()-1;
					          while(k>=0)
					          {
					            if((*curlist)[k].start!=str-start){ok=2;break;}
					            if(!strcmp((char*)(*curlist)[k].name,(char*)OP.rename) && (*curlist)[k].pos==pos)
					            {
					              ok=0;
					              break;
					            }
					            k--;
					          }
					          if(!ok || ok==2)break;
					          if(k<0)curlist=curlist->parent;
					        }
					        if(!ok)
					        {
					          dpf(("Recursion detected! Declain call of %s\n",OP.rename));
					          break;
					        }
					        */
					RegExp *re=(*relib)[(char*)OP.rename];
//        if(matchlist->parent && matchlist->parent->Count())
//        {
					prechar callfrom=matchlist->parent->Last().Get().name;
					int curpos=str-start;
					int k=re->cs.Count()-1;
					int ok=1;

					while (k>=0)
					{
						if (re->cs[k].strpos!=curpos)break;

						if (!strcmp((char*)re->cs[k].name,(char*)callfrom))
						{
							dpf(("Recursive call rejected: %s:%d\n",OP.rename,curpos));
							ok=0;
							break;
						}

						k--;
					}

					if (!ok)break;

//        }
					SCallStackItem csi;
					csi.name=callfrom;
					csi.strpos=curpos;
					re->cs.Append(csi);
					st->op=opLibCall;
					st->pos=op;
					st->startstr=OP.rename;
					st->savestr=str;
					st->cnt=matchlist->Count();
					PushState();
					SMatchListItem *pml;
					int res;

					if (re->havefirst && !re->first[*str])
					{
						dpf(("Fail first char check: %c\n",*str));
						res=0;
					}
					else
					{
						ml.name=OP.rename;
						ml.start=str-start;
						ml.sublist=new MatchList;
						ml.sublist->parent=matchlist;
						pml=&matchlist->Append(ml);
						SMatch mt[10];
						PMatch mtch=mt;

						if (re->bracketscount>10)
#ifdef RE_NO_NEWARRAY
							mtch=static_cast<PMatch>(CreateArray(sizeof(SMatch),
							                                     re->bracketscount, nullptr));

#else
							mtch=new SMatch[re->bracketscount];
#endif // RE_NO_NEWARRAY
						MatchList *mls;
						mls=matchlist;
						re->SetMatchList(ml.sublist);
						int mcnt=re->bracketscount;
						int savecnt=re->stackcount;
						int savestack=re->stackusage;
						PStateStackPage savepage=re->lastpage;
						re->start=start;
						re->reclevel++;
						re->stackusage=0;
						dpf(("Call: %s\n",OP.rename));
#ifdef NAMEDBRACKETS
						MatchHash h;
#endif
						res=re->InnerMatch(str,strend,mtch,mcnt
#ifdef NAMEDBRACKETS
						                   ,&h
#endif
						                  );
						dpf(("Return from: %s - %s\n",OP.rename,res?"ok":"fail"));
						re->cs.Pop(csi);
						matchlist=mls;
						re->reclevel--;
						re->stackusage=savestack;
						pml->end=mtch[0].end;

						if (mtch!=mt)
#ifdef RE_NO_NEWARRAY
							DeleteArray(reinterpret_cast<void**>(&mtch),nullptr);

#else
							delete [] mtch;
#endif // RE_NO_NEWARRAY
						re->lastpage=savepage;
						re->stackcount=savecnt;
						re->stack=re->lastpage->stack;
						re->st=&re->stack[re->stackcount];

						if (!res)
						{
							matchlist->Pop(ml);
							KillMatchList(ml.sublist);
						}
					}

					if (res)
					{
						str=start+pml->end;
					}
					else break;

					continue;
				}
#endif
				case opRegExpEnd:return 1;
			}//switch(op)

		for (;; PopState())
		{
			if (0==(ps=GetState()))return 0;

			//dpf(("ps->op:%s\n",ops[ps->op]));
			switch (ps->op)
			{
				case opAlternative:
				{
					str=ps->savestr;
					op=ps->pos;

					if (OP.alternative.nextalt)
					{
						ps->pos=OP.alternative.nextalt;
					}
					else
					{
						PopState();
					}

					break;
				}
				case opAnyRange:
				case opSymbolRange:
				case opNotSymbolRange:
				case opClassRange:
				case opTypeRange:
				case opNotTypeRange:
				{
					str=ps->savestr-1;
					op=ps->pos;

					if (str<ps->startstr)
					{
						continue;
					}

					ps->savestr=str;
					break;
				}
#ifdef NAMEDBRACKETS
				case opNamedRefRange:
#endif
				case opBackRefRange:
				{
#ifdef NAMEDBRACKETS

//          PMatch m;
					if (ps->op==opBackRefRange)
					{
						m=&match[ps->pos->range.refindex];
					}
					else
					{
						m=hmatch->GetPtr((char*)ps->pos->range.refname);
					}

#else
					m=&match[ps->pos->range.refindex];
#endif
					str=ps->savestr-(m->end-m->start);
					op=ps->pos;

					if (str<ps->startstr)
					{
						continue;
					}

					ps->savestr=str;
					break;
				}
				case opAnyMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;

					if (ps->pos->range.op==opCharAny)
					{
						if (str<strend && *str!=0x0a && *str!=0x0d)
						{
							str++;
							ps->savestr=str;
						}
						else
						{
							continue;
						}
					}
					else
					{
						if (str<strend)
						{
							str++;
							ps->savestr=str;
						}
						else
						{
							continue;
						}
					}

					break;
				}
				case opSymbolMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;

					if (ignorecase)
					{
						if (str<strend && TOLOWER(*str)==OP.symbol)
						{
							str++;
							ps->savestr=str;
						}
						else
						{
							continue;
						}
					}
					else
					{
						if (str<strend && *str==OP.symbol)
						{
							str++;
							ps->savestr=str;
						}
						else
						{
							continue;
						}
					}

					break;
				}
				case opNotSymbolMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;

					if (ignorecase)
					{
						if (str<strend && TOLOWER(*str)!=OP.symbol)
						{
							str++;
							ps->savestr=str;
						}
						else
						{
							continue;
						}
					}
					else
					{
						if (str<strend && *str!=OP.symbol)
						{
							str++;
							ps->savestr=str;
						}
						else
						{
							continue;
						}
					}

					break;
				}
				case opClassMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;

					if (str<strend && GetBit(OP.range.symbolclass,*str))
					{
						str++;
						ps->savestr=str;
					}
					else
					{
						continue;
					}

					break;
				}
				case opTypeMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;

					if (str<strend && ISTYPE(*str,OP.range.type))
					{
						str++;
						ps->savestr=str;
					}
					else
					{
						continue;
					}

					break;
				}
				case opNotTypeMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;

					if (str<strend && ((ISTYPE(*str,OP.range.type))==0))
					{
						str++;
						ps->savestr=str;
					}
					else
					{
						continue;
					}

					break;
				}
#ifdef NAMEDBRACKETS
				case opNamedRefMinRange:
#endif
				case opBackRefMinRange:
				{
					if (ps->max--==0)continue;

					str=ps->savestr;
					op=ps->pos;
#ifdef NAMEDBRACKETS

					if (ps->op==opBackRefMinRange)
					{
						m=&match[OP.range.refindex];
					}
					else
					{
						m=hmatch->GetPtr((char*)OP.range.refname);
					}

#else
					m=&match[OP.range.refindex];
#endif

					if (str+m->end-m->start<strend && StrCmp(str,start+m->start,start+m->end))
					{
						ps->savestr=str;
					}
					else
					{
						continue;
					}

					break;
				}
				case opBracketRange:
				{
					if (ps->pos->range.bracket.index>=0 && brhandler)
					{
						if (
						    !brhandler
						    (
						        brhdata,
						        bhRollBack,
						        ps->pos->range.bracket.index,
						        -1,
						        -1
						    )
						)
						{
							return -1;
						}
					}

					if (ps->min)
					{
						inrangebracket--;
						continue;
					}

					if (ps->forward)
					{
						ps->forward=0;
						op=ps->pos->range.bracket.pairindex;
						inrangebracket--;
						str=ps->savestr;

						if (OP.range.nextalt)
						{
							st->op=opAlternative;
							st->pos=OP.range.bracket.nextalt;
							st->savestr=str;
							PushState();
						}

//            if(OP.bracket.index>=0 && OP.bracket.index<matchcount)
//            {
//              match[OP.bracket.index].end=str-start;
//            }
						break;
					}

					continue;
				}
				case opBracketMinRange:
				{
					if (ps->pos->range.bracket.index>=0 && brhandler)
					{
						if (
						    !brhandler
						    (
						        brhdata,
						        bhRollBack,
						        ps->pos->range.bracket.index,
						        -1,
						        -1
						    )
						)
						{
							return -1;
						}
					}

					if (ps->max--==0)
					{
						inrangebracket--;
						continue;
					}

					if (ps->forward)
					{
						ps->forward=0;
						op=ps->pos;
						str=ps->savestr;

						if (OP.range.bracket.index>=0 && OP.range.bracket.index<matchcount)
						{
							match[OP.range.bracket.index].start=(int)(str-start);
							st->op=opOpenBracket;
							st->pos=op;
							st->min=match[OP.range.bracket.index].start;
							st->max=match[OP.range.bracket.index].end;
							PushState();
						}

						if (OP.range.nextalt)
						{
							st->op=opAlternative;
							st->pos=OP.range.bracket.nextalt;
							st->savestr=str;
							PushState();
						}

						inrangebracket++;
						break;
					}

					inrangebracket--;
					continue;
				}
				case opOpenBracket:
				{
					j=ps->pos->bracket.index;

					if (j>=0 && j<matchcount)
					{
						if (brhandler)
						{
							if (
							    !brhandler
							    (
							        brhdata,
							        bhRollBack,
							        j,
							        match[j].start,
							        match[j].end
							    )
							)
							{
								return -1;
							}
						}

						match[j].start=ps->min;
						match[j].end=ps->max;
					}

					continue;
				}
#ifdef NAMEDBRACKETS
				case opNamedBracket:
				{
					prechar n=ps->pos->nbracket.name;

					if (n && hmatch)
					{
						SMatch sm;
						sm.start=ps->min;
						sm.end=ps->max;
						hmatch->SetItem((char*)n,sm);
					}

					continue;
				}
#endif
				case opLookAhead:
				case opLookBehind:
				{
					continue;
				}
				case opNotLookBehind:
				case opNotLookAhead:
				{
					op=ps->pos->assert.pairindex;
					str=ps->savestr;

					if (ps->forward)
					{
						PopState();
						break;
					}
					else
					{
						continue;
					}
				}
				case opNoReturn:
				{
					return 0;
				}
#ifdef RELIB
				case opLibCall:
				{
					op=ps->pos;
					str=ps->savestr;

					while (matchlist->Count()>ps->cnt)
					{
						matchlist->Pop(ml);
						KillMatchList(ml.sublist);
						ml.sublist=nullptr;
					}

					//PopState();
					continue;
				}
#endif
			}//switch(op)

			break;
		}
	}

	return 1;
}

int RegExp::Match(const RECHAR* textstart,const RECHAR* textend,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                  ,PMatchHash hmatch
#endif
                 )
{
	start=(const prechar)textstart;
	const prechar tempend=(const prechar)textend;

	if (havefirst && !first[*start])return 0;

	TrimTail(tempend);

	if (tempend<start)return 0;

	if (minlength!=0 && tempend-start<minlength)return 0;

	int res=InnerMatch(start,tempend,match,matchcount
#ifdef NAMEDBRACKETS
	                   ,hmatch
#endif
	                  );

	if (res==1)
	{
		for (int i=0; i<matchcount; i++)
		{
			if (match[i].start==-1 || match[i].end==-1 || match[i].start>match[i].end)
			{
				match[i].start=match[i].end=-1;
			}
		}
	}

	return res;
}

int RegExp::MatchEx(const RECHAR* datastart,const RECHAR* textstart,const RECHAR* textend,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                    ,PMatchHash hmatch
#endif
                   )
{
	if (havefirst && !first[(rechar)*textstart])return 0;

	const prechar tempend=(const prechar)textend;

	if ((prechar)datastart==start && (prechar)textend==end)
	{
		tempend=trimend;
	}
	else
	{
		start=(const prechar)datastart;
		TrimTail(tempend);
		trimend=tempend;
	}

	end=(const prechar)textend;

	if (tempend<(const prechar)textstart)return 0;

	if (minlength!=0 && tempend-start<minlength)return 0;

	int res=InnerMatch((const prechar)textstart,tempend,match,matchcount
#ifdef NAMEDBRACKETS
	                   ,hmatch
#endif
	                  );

	if (res==1)
	{
		for (int i=0; i<matchcount; i++)
		{
			if (match[i].start==-1 || match[i].end==-1 || match[i].start>match[i].end)
			{
				match[i].start=match[i].end=-1;
			}
		}
	}

	return res;
}


int RegExp::Match(const RECHAR* textstart,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                  ,PMatchHash hmatch
#endif
                 )
{
	const RECHAR* textend=textstart+strlen(textstart);
	return Match(textstart,textend,match,matchcount
#ifdef NAMEDBRACKETS
	             ,hmatch
#endif
	            );
}



int RegExp::Optimize()
{
	PREOpCode jumps[MAXDEPTH];
	int jumpcount=0;

	if (havefirst)return 1;

#ifdef UNICODE
	first.Reset();
#else
	memset(first,0,sizeof(first));
#endif
	PREOpCode op;
	minlength=0;
	int mlstackmin[MAXDEPTH];
	int mlstacksave[MAXDEPTH];
	int mlscnt=0;

	for (op=code; op; op=op->next)
	{
		switch (op->op)
		{
			case opType:
			case opNotType:
			case opCharAny:
			case opCharAnyAll:
			case opSymbol:
			case opNotSymbol:
			case opSymbolIgnoreCase:
			case opNotSymbolIgnoreCase:
			case opSymbolClass:
				minlength++;
				continue;
			case opSymbolRange:
			case opSymbolMinRange:
			case opNotSymbolRange:
			case opNotSymbolMinRange:
			case opAnyRange:
			case opAnyMinRange:
			case opTypeRange:
			case opTypeMinRange:
			case opNotTypeRange:
			case opNotTypeMinRange:
			case opClassRange:
			case opClassMinRange:
				minlength+=op->range.min;
				break;
#ifdef NAMEDBRACKETS
			case opNamedBracket:
#endif
			case opOpenBracket:
			case opBracketRange:
			case opBracketMinRange:
				mlstacksave[mlscnt]=minlength;
				mlstackmin[mlscnt++]=-1;
				minlength=0;
				continue;
			case opClosingBracket:
			{
				if (op->bracket.pairindex->op>opAssertionsBegin &&
				        op->bracket.pairindex->op<opAsserionsEnd)
				{
					continue;
				}

				if (mlstackmin[mlscnt-1]!=-1 && mlstackmin[mlscnt-1]<minlength)
				{
					minlength=mlstackmin[mlscnt-1];
				}

				switch (op->bracket.pairindex->op)
				{
					case opBracketRange:
					case opBracketMinRange:
						minlength*=op->range.min;
						break;
				}

				minlength+=mlstacksave[--mlscnt];
			} continue;
			case opAlternative:
			{
				if (mlstackmin[mlscnt-1]==-1)
				{
					mlstackmin[mlscnt-1]=minlength;
				}
				else
				{
					if (minlength<mlstackmin[mlscnt-1])
					{
						mlstackmin[mlscnt-1]=minlength;
					}
				}

				minlength=0;
				break;
			}
			case opLookAhead:
			case opNotLookAhead:
			case opLookBehind:
			case opNotLookBehind:
			{
				op=op->assert.pairindex;
				continue;
			}
			case opRegExpEnd:
				op=0;
				break;
		}

		if (!op)break;
	}

	dpf(("minlength=%d\n",minlength));

	for (op=code;; op=op->next)
	{
		switch (OP.op)
		{
			default:
			{
				return 0;
			}
			case opType:
			{
				for (int i=0; i<RE_CHAR_COUNT; i++)if (ISTYPE(i,OP.type))first[i]=1;

				break;
			}
			case opNotType:
			{
				for (int i=0; i<RE_CHAR_COUNT; i++)if (!(ISTYPE(i,OP.type)))first[i]=1;

				break;
			}
			case opSymbol:
			{
				first[OP.symbol]=1;
				break;
			}
			case opSymbolIgnoreCase:
			{
				first[OP.symbol]=1;
				first[TOUPPER(OP.symbol)]=1;
				break;
			}
			case opSymbolClass:
			{
				for (int i=0; i<RE_CHAR_COUNT; i++)
				{
					if (GetBit(OP.symbolclass,i))first[i]=1;
				}

				break;
			}
#ifdef NAMEDBRACKETS
			case opNamedBracket:
#endif
			case opOpenBracket:
			{
				if (OP.bracket.nextalt)
				{
					jumps[jumpcount++]=OP.bracket.nextalt;
				}

				continue;
			}
			case opClosingBracket:
			{
				continue;
			}
			case opAlternative:
			{
				return 0;
			}
			case opSymbolRange:
			case opSymbolMinRange:
			{
				if (ignorecase)
				{
					first[TOLOWER(OP.range.symbol)]=1;
					first[TOUPPER(OP.range.symbol)]=1;
				}
				else
				{
					first[OP.range.symbol]=1;
				}

				if (OP.range.min==0)continue;

				break;
			}
			case opTypeRange:
			case opTypeMinRange:
			{
				for (int i=0; i<RE_CHAR_COUNT; i++)
				{
					if (ISTYPE(i,OP.range.type))first[i]=1;
				}

				if (OP.range.min==0)continue;

				break;
			}
			case opNotTypeRange:
			case opNotTypeMinRange:
			{
				for (int i=0; i<RE_CHAR_COUNT; i++)
				{
					if (!(ISTYPE(i,OP.range.type)))first[i]=1;
				}

				if (OP.range.min==0)continue;

				break;
			}
			case opClassRange:
			case opClassMinRange:
			{
				for (int i=0; i<RE_CHAR_COUNT; i++)
				{
					if (GetBit(OP.range.symbolclass,i))first[i]=1;
				}

				if (OP.range.min==0)continue;

				break;
			}
			case opBracketRange:
			case opBracketMinRange:
			{
				if (OP.range.min==0)return 0;

				if (OP.range.bracket.nextalt)
				{
					jumps[jumpcount++]=OP.range.bracket.nextalt;
				}

				continue;
			}
			//case opLookAhead:
			//case opNotLookAhead:
			//case opLookBehind:
			//case opNotLookBehind:
			case opRegExpEnd:return 0;
		}

		if (jumpcount>0)
		{
			op=jumps[--jumpcount];

			if (OP.op==opAlternative && OP.alternative.nextalt)
			{
				jumps[jumpcount++]=OP.alternative.nextalt;
			}

			continue;
		}

		break;
	}

	havefirst=1;
	return 1;
}

int RegExp::Search(const RECHAR* textstart,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                   ,PMatchHash hmatch
#endif
                  )
{
	const RECHAR* textend=textstart+strlen(textstart);
	return Search(textstart,textend,match,matchcount
#ifdef NAMEDBRACKETS
	              ,hmatch
#endif
	             );
}

int RegExp::Search(const RECHAR* textstart,const RECHAR* textend,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                   ,PMatchHash hmatch
#endif
                  )
{
	start=(const prechar)textstart;
	const prechar str=start;
	const prechar tempend=(prechar)textend;
	TrimTail(tempend);

	if (tempend<start)return 0;

	if (minlength!=0 && tempend-start<minlength)return 0;

	if (code->bracket.nextalt==0 && code->next->op==opDataStart)
	{
		return InnerMatch(start,tempend,match,matchcount
#ifdef NAMEDBRACKETS
		                  ,hmatch
#endif
		                 );
	}

	if (code->bracket.nextalt==0 && code->next->op==opDataEnd && code->next->next->op==opClosingBracket)
	{
		matchcount=1;
		match[0].start=(int)(textend-textstart);
		match[0].end=match[0].start;
		return 1;
	}

	int res=0;

	if (havefirst)
	{
		do
		{
			while (!first[*str] && str<tempend)str++;

			if (0!=(res=InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
			                       ,hmatch
#endif
			                      )))
			{
				break;
			}

			str++;
		}
		while (str<tempend);

		if (!res && InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
		                       ,hmatch
#endif
		                      ))
		{
			res=1;
		}
	}
	else
	{
		do
		{
			if (0!=(res=InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
			                       ,hmatch
#endif
			                      )))
			{
				break;
			}

			str++;
		}
		while (str<=tempend);
	}

	if (res==1)
	{
		for (int i=0; i<matchcount; i++)
		{
			if (match[i].start==-1 || match[i].end==-1 || match[i].start>match[i].end)
			{
				match[i].start=match[i].end=-1;
			}
		}
	}

	return res;
}

int RegExp::SearchEx(const RECHAR* datastart,const RECHAR* textstart,const RECHAR* textend,PMatch match,int& matchcount
#ifdef NAMEDBRACKETS
                     ,PMatchHash hmatch
#endif
                    )
{
	start=(const prechar)datastart;
	const prechar str=(const prechar)textstart;
	const prechar tempend=(const prechar)textend;
	TrimTail(tempend);

	if (tempend<start)return 0;

	if (minlength!=0 && tempend-start<minlength)return 0;

	if (code->bracket.nextalt==0 && code->next->op==opDataStart)
	{
		return InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
		                  ,hmatch
#endif
		                 );
	}

	if (code->bracket.nextalt==0 && code->next->op==opDataEnd && code->next->next->op==opClosingBracket)
	{
		matchcount=1;
		match[0].start=(int)(textend-datastart);
		match[0].end=match[0].start;
		return 1;
	}

	int res=0;

	if (havefirst)
	{
		do
		{
			while (!first[*str] && str<tempend)str++;

			if (0!=(res=InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
			                       ,hmatch
#endif
			                      )))
			{
				break;
			}

			str++;
		}
		while (str<tempend);

		if (!res && InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
		                       ,hmatch
#endif
		                      ))
		{
			res=1;
		}
	}
	else
	{
		do
		{
			if (0!=(res=InnerMatch(str,tempend,match,matchcount
#ifdef NAMEDBRACKETS
			                       ,hmatch
#endif
			                      )))
			{
				break;
			}

			str++;
		}
		while (str<=tempend);
	}

	if (res==1)
	{
		for (int i=0; i<matchcount; i++)
		{
			if (match[i].start==-1 || match[i].end==-1 || match[i].start>match[i].end)
			{
				match[i].start=match[i].end=-1;
			}
		}
	}

	return res;
}

void RegExp::TrimTail(const prechar& strend)
{
	if (havelookahead)return;

	if (!code || code->bracket.nextalt)return;

	PREOpCode op=code->bracket.pairindex->prev;

	while (OP.op==opClosingBracket)
	{
		if (OP.bracket.pairindex->op!=opOpenBracket)return;

		if (OP.bracket.pairindex->bracket.nextalt)return;

		op=op->prev;
	}

	strend--;

	switch (OP.op)
	{
		case opSymbol:
		{
			while (strend>=start && *strend!=OP.symbol)strend--;

			break;
		}
		case opNotSymbol:
		{
			while (strend>=start && *strend==OP.symbol)strend--;

			break;
		}
		case opSymbolIgnoreCase:
		{
			while (strend>=start && TOLOWER(*strend)!=OP.symbol)strend--;

			break;
		}
		case opNotSymbolIgnoreCase:
		{
			while (strend>=start && TOLOWER(*strend)==OP.symbol)strend--;

			break;
		}
		case opType:
		{
			while (strend>=start && !(ISTYPE(*strend,OP.type)))strend--;

			break;
		}
		case opNotType:
		{
			while (strend>=start && ISTYPE(*strend,OP.type))strend--;

			break;
		}
		case opSymbolClass:
		{
			while (strend>=start && !GetBit(OP.symbolclass,*strend))strend--;

			break;
		}
		case opSymbolRange:
		case opSymbolMinRange:
		{
			if (OP.range.min==0)break;

			if (ignorecase)
			{
				while (strend>=start && *strend!=OP.range.symbol)strend--;
			}
			else
			{
				while (strend>=start && TOLOWER(*strend)!=OP.range.symbol)strend--;
			}

			break;
		}
		case opNotSymbolRange:
		case opNotSymbolMinRange:
		{
			if (OP.range.min==0)break;

			if (ignorecase)
			{
				while (strend>=start && *strend==OP.range.symbol)strend--;
			}
			else
			{
				while (strend>=start && TOLOWER(*strend)==OP.range.symbol)strend--;
			}

			break;
		}
		case opTypeRange:
		case opTypeMinRange:
		{
			if (OP.range.min==0)break;

			while (strend>=start && !(ISTYPE(*strend,OP.range.type)))strend--;

			break;
		}
		case opNotTypeRange:
		case opNotTypeMinRange:
		{
			if (OP.range.min==0)break;

			while (strend>=start && ISTYPE(*strend,OP.range.type))strend--;

			break;
		}
		case opClassRange:
		case opClassMinRange:
		{
			if (OP.range.min==0)break;

			while (strend>=start && !GetBit(OP.range.symbolclass,*strend))strend--;

			break;
		}
		default:break;
	}

	strend++;
}

void RegExp::CleanStack()
{
	PStateStackPage tmp=firstpage->next,tmp2;

	while (tmp)
	{
		tmp2=tmp->next;
#ifdef RE_NO_NEWARRAY
		DeleteArray(reinterpret_cast<void**>(&tmp->stack),nullptr);
#else
		delete [] tmp->stack;
#endif // RE_NO_NEWARRAY
		delete tmp;
		tmp=tmp2;
	}
}

#ifndef UNICODE
void RegExp::SetLocaleInfo(prechar newlc,prechar newuc,prechar newchartypes
#if defined(RE_EXTERNAL_CTYPE)
                           , prechar newcharbits
#endif
                          )
{
#ifndef RE_EXTERNAL_CTYPE
	memcpy(lc,newlc,256);
	memcpy(uc,newuc,256);
	memcpy(chartypes,newchartypes,256);
#else
	lc=newlc;
	uc=newuc;
	chartypes=newchartypes;
#endif
	int i,j=0,k=1;
	memset(charbits,0,sizeof(charbits));

	for (i=0; i<256; i++)
	{
		if (ISDIGIT(i)) {charbits[j]|=k;}

		if (ISSPACE(i)) {charbits[32+j]|=k;}

		if (ISWORD(i)) {charbits[64+j]|=k;}

		if (ISLOWER(i)) {charbits[96+j]|=k;}

		if (ISUPPER(i)) {charbits[128+j]|=k;}

		if (ISALPHA(i)) {charbits[160+j]|=k;}

		k<<=1;

		if (k==256) {k=1; j++;}
	}
}
#endif //UNICODE

#ifdef RELIB

int RELibMatch(RELib& relib,MatchList& ml,const char* name,const char* start)
{
	return RELibMatch(relib,ml,name,start,start+strlen((char*)start));
}

int RELibMatch(RELib& relib,MatchList& ml,const char* name,const char* start,const char* end)
{
	char* k;
	RegExp *re;
	relib.First();

	while (relib.Next(k,re))
	{
		re->ResetRecursion();
	}

	if (!relib.Exists((char*)name))return 0;

	int cnt=relib[(char*)name]->GetBracketsCount();
	PMatch m=new SMatch[cnt];
	PMatchList pml=new MatchList;
	SMatchListItem li;
	li.name=(const prechar)name;
	li.sublist=pml;
	li.start=0;
	ml.Append(li);
	ml.parent=nullptr;
	pml->parent=&ml;
	relib[(char*)name]->SetMatchList(pml);
#ifdef NAMEDBRACKETS
	MatchHash h;
#endif
	int res=relib[(char*)name]->Match(start,end,m,cnt
#ifdef NAMEDBRACKETS
	                                  ,&h
#endif
	                                 );
	ml.First().Get().start=m[0].start;
	ml.First().Get().end=m[0].end;
	delete [] m;
	return res;
}

#endif
