#pragma once

#ifndef X_SCRIPT_BINDS_IO_H_
#define X_SCRIPT_BINDS_IO_H_

X_NAMESPACE_DECLARE(core,
struct IFileSys;
struct XFile;
)

X_NAMESPACE_BEGIN(script)

class XBinds_Io;
class XBinds_Io_File : public XScriptableBase
{
	friend class XBinds_Io;
public:
	XBinds_Io_File(IScriptSys* pSS, ICore* pCore);
	~XBinds_Io_File() X_OVERRIDE;

private:
	void init(IScriptSys* pSS, ICore* pCore);

	int write(IFunctionHandler* pH);
	int read(IFunctionHandler* pH);
	int seek(IFunctionHandler* pH);
	int close(IFunctionHandler* pH);

private:
	int readNumber(IFunctionHandler* pH, core::XFile* pFile);
	int readLine(IFunctionHandler* pH, core::XFile* pFile, bool keepEol = false);
	int readBytes(IFunctionHandler* pH, core::XFile* pFile, uint32_t numBytes);

protected:
	core::IFileSys* pFileSys_;

	static int garbageCollect(IFunctionHandler* pH, void* pBuffer, int size);
	static core::XFile* getFile(IFunctionHandler* pH);
};


class XBinds_Io : public XScriptableBase
{
public:
	XBinds_Io(IScriptSys* pSS, ICore* pCore);
	~XBinds_Io() X_OVERRIDE;

private:
	void init(IScriptSys* pSS, ICore* pCore);

	int openFile(IFunctionHandler* pH);
	int closeFile(IFunctionHandler* pH);

	SmartScriptTable WrapFileReturn(core::XFile* pFile);

private:
	core::IFileSys* pFileSys_;

	XBinds_Io_File file_;
};


X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_IO_H_