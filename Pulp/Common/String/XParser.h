#pragma once

#ifndef X_STR_PARSER_H_
#define X_STR_PARSER_H_

#include "Lexer.h"

#include "Containers\HashMap.h"

X_NAMESPACE_BEGIN(core)

//
//	This is a Lexer that supports macros
//

struct MacroDefine
{
	MacroDefine() : numParams(0), pTokens(nullptr), pParms(nullptr) {}

	core::string name;
	int32_t numParams;
	XLexToken* pTokens;
	XLexToken* pParms;
};



class XParser
{
	typedef XLexer::LexFlags LexFlags;
public:
	explicit XParser(MemoryArenaBase* arena);
	XParser(LexFlags flags, MemoryArenaBase* arena);
	XParser(const char* startInclusive, const char* endExclusive, const char* name, LexFlags flags, MemoryArenaBase* arena);
	~XParser();

	// load me up!
	bool LoadMemory(const char* startInclusive, const char* endExclusive, const char* name);

	int	ReadToken(XLexToken& token);
	int ExpectTokenString(const char* string);
	int	ExpectTokenType(int type, int subtype, XLexToken& token);

	int ParseInt();
	bool ParseBool();
	float ParseFloat();

	void UnreadToken(const XLexToken& token);
	const int GetLineNumber(void);
	bool isEOF(void) const;

	X_INLINE void setFlags(LexFlags flags) {
		flags_ = flags;
	}
	X_INLINE LexFlags getFlags(void) {
		return flags_;
	}

private:
	void Error(const char *str, ...);
	void Warning(const char *str, ...);

	void freeSource(void);

	bool ReadSourceToken(XLexToken& token);
	bool UnreadSourceToken(const XLexToken& token);
	bool ReadDirective(void);

	bool isInCache(const char ch) const;
	void addToCache(const char ch);

	bool Directive_define(void);
	bool Directive_include(void);

	bool CheckTokenString(const char* string);
	int FindDefineParm(MacroDefine* define, const XLexToken& token);
	int ReadDefineParms(MacroDefine* pDefine, XLexToken** parms, int maxparms);

	int	ReadLine(XLexToken& token);

	bool ExpandDefine(XLexToken& deftoken, MacroDefine* pDefine, 
		XLexToken*& firsttoken, XLexToken*& lasttoken);

	bool ExpandDefineIntoSource(XLexToken& token, MacroDefine* pDefine);

	MacroDefine* FindDefine(XLexToken& token) const;
	void addDefinetoHash(MacroDefine* define);

private:
	typedef core::HashMap<core::string, MacroDefine*> MacroMap;

	core::string filename_;

	MacroMap macros_;
	LexFlags flags_;

	MemoryArenaBase* arena_;
	XLexer* pLexer_;
	XLexToken* pTokens_;


	uint8_t macroCharCache[255];
};

X_NAMESPACE_END

#endif // !X_STR_PARSER_H_