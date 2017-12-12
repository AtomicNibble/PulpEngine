#include "stdafx.h"
#include "ScriptBinds_io.h"

#include <IFileSys.h>

#include "ScriptTable.h"
#include "TableDump.h"

#include "Containers\Array.h"


X_NAMESPACE_BEGIN(script)

#define X_IO_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Io, &XBinds_Io::func>(this); \
	registerFunction(#func, Delegate); \
}

#define X_IO_FILE_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Io_File, &XBinds_Io_File::func>(this); \
	registerFunction(#func, Delegate); \
}

XBinds_Io_File::XBinds_Io_File()
{

}

XBinds_Io_File::~XBinds_Io_File()
{

}

void XBinds_Io_File::init(IScriptSys* pSS, ICore* pCore, int paramIdOffset)
{
	XScriptableBase::init(pSS, pCore, paramIdOffset);

	X_ASSERT_NOT_NULL(pCore->GetIFileSys());

	pFileSys_ = pCore->GetIFileSys();

	X_IO_FILE_REG_FUNC(write);
	X_IO_FILE_REG_FUNC(read);
	X_IO_FILE_REG_FUNC(seek);
	X_IO_FILE_REG_FUNC(close);

#if 0
	IScriptTable::XUserFunctionDesc fd;
	fd.pUserDataFunc = XBinds_Io_File::garbageCollect;
	fd.sFunctionName = "__gc";
	fd.userDataSize = sizeof(core::XFile*);
	fd.pDataBuffer = this;
	fd.sGlobalName = "<gc-file>";
	fd.sFunctionParams = "()";
	GetMethodsTable()->AddFunction(fd);
#endif
}

core::XFile* XBinds_Io_File::getFile(IFunctionHandler* pH, int index, bool nullPointer)
{
	X_ASSERT_NOT_NULL(pH);

	SmartScriptTable tbl;
	Type::Enum type = pH->getParamType(index);

	if (type == Type::POINTER) // this is a script handle
	{
		Handle fileHandle;
		if (!pH->getParam(index, fileHandle)) {
			return nullptr;
		}

		return static_cast<core::XFile*>(fileHandle.pPtr);
	}
	else if (pH->getParam(1, tbl))
	{
		void* pPtr = (static_cast<XScriptTable*>(tbl.GetPtr()))->getUserData();
		if (!pPtr) {
			return nullptr;
		}

		core::XFile* pFile = *static_cast<core::XFile**>(pPtr);

		if (nullPointer) {
			*static_cast<core::XFile**>(pPtr) = nullptr;
		}

		return pFile;
	}

	return nullptr;
}


int XBinds_Io_File::write(IFunctionHandler* pH)
{
	core::XFile* pFile;
	int arg, numArgs;

	if ((pFile = getFile(pH)) != nullptr)
	{
		numArgs = pH->getParamCount() - 1;
		for (arg = 2; numArgs--; arg++)
		{
			ScriptValue value;
			pH->getParamAny(arg, value);
			switch (value.getType())
			{
				case Type::STRING:
			//	pFile->writeString(value.str);
			// no null term Plz!
				pFile->write(value.str_.pStr, safe_static_cast<uint32_t>(value.str_.len));
				break;
				case Type::NUMBER:
			//	pFile->writeObj(value.number);
			// write it as a string.
				pFile->printf(LUA_NUMBER_FMT, value.number_);
				break;
				default:
					break;
			}
		}
	}

	return pH->endFunction();
}

int XBinds_Io_File::read(IFunctionHandler* pH)
{
	// <foramt>
	core::XFile* pFile;

/*
	"*n": reads a number; this is the only format that returns a number instead of a string.
	"*a": reads the whole file, starting at the current position.On end of file, it returns the empty string.
	"*l" : reads the next line skipping the end of line, returning nil on end of file.This is the default format.
	"*L" : reads the next line keeping the end of line(if present), returning nil on end of file.
	number : reads a string with up to this number of bytes, returning nil on end of file. If number is zero, 
			 it reads nothing and returns an empty string, or nil on end of file.
*/
	int arg, numArgs;
	int total;

	total = 0;

	if ((pFile = getFile(pH)) != nullptr)
	{
		numArgs = pH->getParamCount() - 1;
		if (numArgs == 0)
		{
			// read line mode
			readLine(pH, pFile);

		}
		else
		{
			// need to check for stack space.
			for (arg = 2; numArgs--; arg++)
			{
				const char* mode;

				if (pH->getParamType(arg) == Type::NUMBER)
				{
					// read this number of bytes.
					// not gonna use float(aka lua number) since who the flying
					// gypsy fuck is gonna open a >2gb file via lua o.o !!
					int numbytes;
					if (pH->getParam(arg, numbytes))
					{

					}
				}
				if (pH->getParam(arg,mode))
				{
					if (core::strUtil::strlen(mode) == 2 && mode[0] == '*')
					{
						char m = mode[1];
						switch (m)
						{
						case 'n':
							total += readNumber(pH, pFile);
							break;
						case 'a':
							total += readBytes(pH, pFile, std::numeric_limits<uint32_t>::max());
							break;
						case 'l':
							total += readLine(pH, pFile);
							break;
						case 'L':
							total += readLine(pH, pFile, true);
							break;
						default:
							pScriptSys_->onScriptError("Unknown file:read mode: \"%s\" valid modes: *n,*a,*l,*L", mode);
							break;
						}

					}
					else
					{
						pScriptSys_->onScriptError("Unknown file:read mode: \"%s\" valid modes: *n,*a,*l,*L", mode);
					}
				}
			}
		}
	}

	return total;
}

int XBinds_Io_File::seek(IFunctionHandler* pH)
{
	// <where(str)> <offset(int)>
	using namespace core;

	XFile* pFile;
	SeekMode::Enum mode;
	const char* modeStr;
	int offset;

	mode = SeekMode::CUR;
	modeStr = nullptr;
	offset = 0;

	if (pH->getParam(1, modeStr)) {
		pH->getParam(2, offset);
	}
	else
	{
		modeStr = "cur";
	}

	struct SeekMap {
		const char* str;
		SeekMode::Enum mode;
	};

	SeekMap seekLookup[] = {
		{ "cur", SeekMode::CUR },
		{ "set", SeekMode::SET },
		{ "end", SeekMode::END }
	};

	const uint32_t numModes = SeekMode::FLAGS_COUNT;
	uint32_t i;

	for (i = 0; i < numModes; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(modeStr, seekLookup[i].str))
		{
			mode = seekLookup[i].mode;
			break;
		}

		if (i == numModes)
		{ 
			pScriptSys_->onScriptError("Unknown file:seek mode: \"%s\" valid modes: cur,set,end", mode);
			return pH->endFunction();
		}
	}

	if ((pFile = getFile(pH)) != nullptr)
	{
		// if random access is not set, file system will print error.
		// may add a check here, and handle it diffrently.
		pFile->seek(offset, mode);
	}

	return pH->endFunction();
}

int XBinds_Io_File::close(IFunctionHandler* pH)
{
	core::XFile* pFile;

	if ((pFile = getFile(pH,1,true)) != nullptr)
	{
		if (pFile) {
			pFileSys_->closeFile(pFile);
		}
	}

	return pH->endFunction();
}

// ------------------------- read util -----------------------------

int XBinds_Io_File::readNumber(IFunctionHandler* pH, core::XFile* pFile)
{
	X_ASSERT_NOT_NULL(pFile);
	X_ASSERT_NOT_IMPLEMENTED();

	return pH->endFunction();
}

int XBinds_Io_File::readLine(IFunctionHandler* pH, core::XFile* pFile, bool keepEol)
{
	X_ASSERT_NOT_NULL(pFile);
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(keepEol);
	return pH->endFunction();
}

int XBinds_Io_File::readBytes(IFunctionHandler* pH, core::XFile* pFile, uint32_t numBytes)
{
	X_ASSERT_NOT_NULL(pFile);

	uint32_t remaning;
	uint32_t readSize, numRead;

	// we either read less or == what's left.
	remaning = safe_static_cast<uint32_t>(pFile->remainingBytes());
	readSize = core::Min<uint32_t>(remaning, numBytes);


	core::Array<char> temp(g_ScriptArena);
	temp.resize(readSize + 1);

	numRead = safe_static_cast<uint32_t, size_t>(pFile->read(temp.ptr(), readSize));

	temp[numRead] = '\0';

	// mem is freed after on scope exit.
	// lua will of made it's own copy by then.
	return pH->endFunctionAny(temp.ptr());
}

// --------------------------- static ------------------------------

int XBinds_Io_File::garbageCollect(IFunctionHandler* pH, void* pBuffer, int size)
{
	X_ASSERT(size == sizeof(core::XFile*), "invalid buffer size")(pBuffer, size);

	if (pBuffer)
	{
		core::XFile* pfile = *static_cast<core::XFile**>(pBuffer);

		if (gEnv && gEnv->pFileSys) {
			gEnv->pFileSys->closeFile(pfile);
		}
		else
		{

			pH->getIScriptSystem()->onScriptError( "Failed to close file");	
		}
	}


	return pH->endFunction();
}

// --------------------------------------------------

XBinds_Io::XBinds_Io()
{

}

XBinds_Io::~XBinds_Io()
{

}

void XBinds_Io::init(IScriptSys* pSS, ICore* pCore, int paramIdOffset)
{
	XScriptableBase::init(pSS, pCore, paramIdOffset);

	X_ASSERT_NOT_NULL(pCore_);
	X_ASSERT_NOT_NULL(pCore->GetIFileSys());

	pCore_ = pCore;
	pFileSys_ = pCore->GetIFileSys();

	setGlobalName("io");


	X_IO_REG_FUNC(openFile);
	X_IO_REG_FUNC(closeFile);


	file_.init(pSS, pCore, paramIdOffset);
}


int XBinds_Io::openFile(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS_MIN(1);

	using namespace core;

// The c fopen docs, just gonna mimic these.
//	"r"	read: 
//			Open file for input operations. The file must exist.
//	"w"	write : 
//			Create an empty file for output operations. If a file with the same name already exists, 
//			its contents are discarded and the file is treated as a new empty file.
//	"a"	append : 
//			Open file for output at the end of a file.
//			Output operations always write data at the end of the file, expanding it.
//			Repositioning operations(fseek, fsetpos, rewind) are ignored.
//			The file is created if it does not exist.
//	"r+" read / update : 
//			Open a file for update(both for input and output). The file must exist.
//	"w+" write / update : 
//			Create an empty file and open it for update(both for input and output).
//			If a file with the same name already exists its contents are discarded and the 
//			file is treated as a new empty file.
//	"a+" append / update : 
//			Open a file for update(both for input and output) with all output 
//			operations writing data at the end of the file. Repositioning 
//			operations(fseek, fsetpos, rewind) affects the next input operations, 
//			but output operations move the position back to the end of file.
//			The file is created if it does not exist.

	struct ModeMap
	{
		const char* mode;
		core::fileModeFlags flags;
	};

	ModeMap modeLookup[] = {
		// allow RANDOM_ACCESS for the plebs :)
		{ "r", fileMode::READ | fileMode::RANDOM_ACCESS },
		{ "w", fileMode::WRITE | fileMode::RANDOM_ACCESS | fileMode::RECREATE },
		{ "a", fileMode::APPEND },
		{ "r+", fileMode::WRITE | fileMode::READ | fileMode::RANDOM_ACCESS },
		{ "w+", fileMode::WRITE | fileMode::READ | fileMode::RANDOM_ACCESS | fileMode::RECREATE },
	//	{ "a+", fileMode::READ | fileMode::RANDOM_ACCESS }, not supported
	};

	static const size_t numModes = sizeof(modeLookup) / sizeof(modeLookup[0]);

	const char* fileName = nullptr;
	const char* mode = nullptr;
	size_t i;
	core::Path<char> path;

	core::fileModeFlags flags = core::fileMode::READ | fileMode::RANDOM_ACCESS;

	pH->getParam(1, fileName);
	if (pH->getParam(2, mode))
	{		
		for (i = 0; i < numModes; i++)
		{
			if (core::strUtil::IsEqualCaseInsen(mode, modeLookup[i].mode))
			{
				flags = modeLookup[i].flags;
				break;
			}
		}

		if (i == numModes)
		{
			pScriptSys_->onScriptError("Unknown fileopen mode: \"%s\" valid modes: r,w,a,r+,w+", mode);
		}
	}

	// make a path.
	// should i just only allow filenames?
	// for now :D !
	path.append(core::strUtil::FileName(fileName));

	if (path.length() == 0)
	{
		pScriptSys_->onScriptError("invalid filename: %s", fileName);
		return pH->endFunction();
	}


	XFile* pFile = pFileSys_->openFile(path.c_str(), flags);

	if (pFile) {
		return pH->endFunction(WrapFileReturn(pFile));
	}

	return pH->endFunction();
}

SmartScriptTable XBinds_Io::WrapFileReturn(core::XFile* pFile)
{
	SmartScriptTable userData = pScriptSys_->createUserData(&pFile, sizeof(pFile));
	

	// files have to be explicitly closed.
	// unless i can somehow get a pointer to table userdata
	// for the file and store a pointer to that.
	// so i can check for null.
#if 0
	IScriptTable::XUserFunctionDesc fd;
	fd.pUserDataFunc = XBinds_Io_File::garbageCollect;
	fd.sFunctionName = "__gc";
	fd.userDataSize = sizeof(core::XFile*);
	fd.pDataBuffer = &pFile;
	fd.sGlobalName = "<gc-file>";
	fd.sFunctionParams = "()";
	file_.GetMethodsTable()->AddFunction(fd);
#endif


//	(static_cast<XScriptTable*>(userData.GetPtr()))->Delegate(file_.GetMethodsTable());

	return userData;
}


int XBinds_Io::closeFile(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	using namespace core;

	core::XFile* pFile;

	if ((pFile = XBinds_Io_File::getFile(pH,1,true)) != nullptr)
	{
		pFileSys_->closeFile(pFile);
	}

	return pH->endFunction();
}




X_NAMESPACE_END
