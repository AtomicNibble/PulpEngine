#pragma once

#ifndef _X_CMDARGS_X_H_
#define _X_CMDARGS_X_H_

X_NAMESPACE_BEGIN(core)

template<typename TChar = char>
class CmdArgs 
{
	static const size_t		MAX_COMMAND_ARGS = 64;
	static const size_t		MAX_COMMAND_STRING = 4096;

public:
	CmdArgs(void);
	explicit CmdArgs(const TChar* text);

	void clear(void);

	size_t getArgc(void) const;
	const TChar* getArgv(size_t arg) const;

private:
	void tokenize(const TChar* pText);

private:
	size_t	argc_;							
	TChar*	argv_[MAX_COMMAND_ARGS];		
	TChar	tokenized_[MAX_COMMAND_STRING];	
};


X_NAMESPACE_END

#endif // _X_CMDARGS_X_H_