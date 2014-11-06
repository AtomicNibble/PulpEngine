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
	const char* name;

};


class XParser
{
public:
	XParser();
	~XParser();

	int	ReadToken(XLexToken& token);
	
	void UnreadToken(const XLexToken& token);

	const int GetLineNumber(void);

	bool isEOF(void) const;

private:
	bool ReadSourceToken(XLexToken& token);
	bool ReadDirective(void);

	bool isInCache(const char ch) const;
	void addToCache(const char ch);

	bool Directive_define(void);
	bool Directive_include(void);

	MacroDefine* FindDefine(XLexToken& token) const;
private:
	static const int numLetters = 52;
	uint8_t macroCharCache[numLetters];

	typedef core::HashMap<core::string, MacroDefine*> MacroMap;

	MacroMap macros_;

	XLexer* pLexer_;
};

X_NAMESPACE_END

#endif // !X_STR_PARSER_H_