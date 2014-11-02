#pragma once


#ifndef X_STRING_LEXER_H_
#define X_STRING_LEXER_H_

#include <String\StringUtil.h>
#include <Util\FlagsMacros.h>

X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(LexFlag)(
	NOERRORS,					// don't print any errors
	NOWARNINGS,					// don't print any warnings
	NOFATALERRORS,				// errors aren't fatal
	NOSTRINGCONCAT,				// multiple strings seperated by whitespaces are not concatenated
	NOSTRINGESCAPECHARS,		// no escape characters inside strings
	NODOLLARPRECOMPILE,			// don't use the $ sign for precompilation
	NOBASEINCLUDES,				// don't include files embraced with < >
	ALLOWPATHNAMES,				// allow path seperators in names
	ALLOWNUMBERNAMES,			// allow names to start with a number
	ALLOWIPADDRESSES,			// allow ip addresses to be parsed as numbers
	ALLOWFLOATEXCEPTIONS,		// allow float exceptions like 1.#INF or 1.#IND to be parsed
	ALLOWMULTICHARLITERALS,		// allow multi character literals
	ALLOWBACKSLASHSTRINGCONCAT,	// allow multiple strings seperated by '\' to be concatenated
	ONLYSTRINGS					// parse as whitespace deliminated strings (quoted strings keep quotes)
);


// token types
#define TT_STRING					1		// string
#define TT_LITERAL					2		// literal
#define TT_NUMBER					3		// number
#define TT_NAME						4		// name
#define TT_PUNCTUATION				5		// punctuation

// number sub types
#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000		// set if intvalue and floatvalue are valid


#define PUNCTABLE
// punctuation ids
#define P_RSHIFT_ASSIGN				1
#define P_LSHIFT_ASSIGN				2
#define P_PARMS						3
#define P_PRECOMPMERGE				4

#define P_LOGIC_AND					5
#define P_LOGIC_OR					6
#define P_LOGIC_GEQ					7
#define P_LOGIC_LEQ					8
#define P_LOGIC_EQ					9
#define P_LOGIC_UNEQ				10

#define P_MUL_ASSIGN				11
#define P_DIV_ASSIGN				12
#define P_MOD_ASSIGN				13
#define P_ADD_ASSIGN				14
#define P_SUB_ASSIGN				15
#define P_INC						16
#define P_DEC						17

#define P_BIN_AND_ASSIGN			18
#define P_BIN_OR_ASSIGN				19
#define P_BIN_XOR_ASSIGN			20
#define P_RSHIFT					21
#define P_LSHIFT					22

#define P_POINTERREF				23
#define P_CPP1						24
#define P_CPP2						25
#define P_MUL						26
#define P_DIV						27
#define P_MOD						28
#define P_ADD						29
#define P_SUB						30
#define P_ASSIGN					31

#define P_BIN_AND					32
#define P_BIN_OR					33
#define P_BIN_XOR					34
#define P_BIN_NOT					35

#define P_LOGIC_NOT					36
#define P_LOGIC_GREATER				37
#define P_LOGIC_LESS				38

#define P_REF						39
#define P_COMMA						40
#define P_SEMICOLON					41
#define P_COLON						42
#define P_QUESTIONMARK				43

#define P_PARENTHESESOPEN			44
#define P_PARENTHESESCLOSE			45
#define P_BRACEOPEN					46
#define P_BRACECLOSE				47
#define P_SQBRACKETOPEN				48
#define P_SQBRACKETCLOSE			49
#define P_BACKSLASH					50

#define P_PRECOMP					51
#define P_DOLLAR					52


// punctuation
typedef struct punctuation_s
{
	char *p;						// punctuation character(s)
	int n;							// punctuation id
} punctuation_t;

class XLexer;

class XLexToken
{
public:
	XLexToken() : start_(nullptr), end_(nullptr) { Init(); }
	XLexToken(const char* start, const char* end) : start_(start), end_(end) { Init(); }

	size_t length() const { return end_ - start_; }

	const char* begin() const { return start_; }
	const char* end() const { return end_; }

	inline bool isEqual(const char* str) {
		return strUtil::IsEqual(start_, end_, str);
	}

	double			GetDoubleValue(void);				// double value of TT_NUMBER
	float			GetFloatValue(void);				// float value of TT_NUMBER
	unsigned long	GetUnsignedLongValue(void);		// unsigned long value of TT_NUMBER
	int				GetIntValue(void);

//	XLexToken& operator=(const XLexToken& oth) {
//		return *this;
//	}

private:
	friend class XLexer;

	void NumberValue(void);

	void Reset()
	{
		start_ = nullptr;
		end_ = nullptr;
		Init();
	}

	void Init()
	{
		line = -1;
		linesCrossed = -1;
		flags = 0;

		intvalue = 0;
		floatvalue = 0.f;

		type = 0;
		subtype = 0;
	}

public:

	int		type;				// token type
	int		subtype;			// token sub type
	int		line;				// line in script the token was on
	int		linesCrossed;		// number of lines crossed in white space before token
	int		flags;				// token flags, used for recursive defines

	long		intvalue;	// integer value
	double		floatvalue;

	const char* start_;
	const char* end_;
};

class XLexer
{
	XLexer() {}
public:
	typedef Flags<LexFlag> LexFlags;

	XLexer(const char* startInclusive, const char* endExclusive);

	int	ReadToken(XLexToken& token);

	int				ExpectTokenString(const char* string);
	int				ExpectTokenType(int type, int subtype, XLexToken& token);

	int				PeekTokenString(const char *string);
	int				PeekTokenType(int type, int subtype, XLexToken& token);

	int				SkipUntilString(const char *string);
	int				SkipRestOfLine(void);

	int				ParseInt(void);
	bool			ParseBool(void);
	float			ParseFloat();

	// parse matrices with floats
	int				Parse1DMatrix(int x, float *m);
	int				Parse2DMatrix(int y, int x, float *m);
	int				Parse3DMatrix(int z, int y, int x, float *m);

	void			UnreadToken(const XLexToken& token);
	// read a token only if on the same line
	int				ReadTokenOnLine(XLexToken& token);

	const int		GetLineNumber(void);

	void setFlags(LexFlags flags) {
		flags_ = flags;
	}
	void Error(const char *str, ...);
	void Warning(const char *str, ...);

private:
	void SetPunctuations(const punctuation_t *p);
	void CreatePunctuationTable(const punctuation_t *punctuations);
	const char* GetPunctuationFromId(int id);

	int ReadWhiteSpace(void);

	int	ReadEscapeCharacter(char *ch);
	int	ReadString(XLexToken& token, int quote);
	int	ReadName(XLexToken& token);
	int	ReadNumber(XLexToken& token);
	int	ReadPunctuation(XLexToken& token);
	int	ReadPrimitive(XLexToken& token);

	X_INLINE int CheckString(const char *str) const;

	const char* start_;
	const char* end_;
	const char* current_;
	const char* lastp_;
	int			curLine_;
	int			lastLine_;				// line before reading token
	LexFlags	flags_;

	XLexToken	token;
	int			tokenavailable;

	const punctuation_t *punctuations;		// the punctuations used in the script
	int *			punctuationtable;		// ASCII table with punctuations
	int *			nextpunctuation;
};


X_INLINE const int XLexer::GetLineNumber(void)
{
	return curLine_;
}

X_NAMESPACE_END

#endif // X_STRING_LEXER_H_