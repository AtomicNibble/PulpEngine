#pragma once

#include "ScriptBinds.h"

X_NAMESPACE_DECLARE(core,
                    struct IFileSys;
                    struct XFile;)

X_NAMESPACE_BEGIN(script)

class XScriptableBase;
class XScriptSys;

class XBinds_Io;
class XBinds_Io_File : public XScriptBindsBase
{
    friend class XBinds_Io;

public:
    XBinds_Io_File(XScriptSys* pSS);
    ~XBinds_Io_File();

private:
    void bind(ICore* pCore) X_FINAL;

    int write(IFunctionHandler* pH);
    int read(IFunctionHandler* pH);
    int seek(IFunctionHandler* pH);
    int close(IFunctionHandler* pH);

private:
    int readNumber(IFunctionHandler* pH, core::XFile* pFile);
    int readLine(IFunctionHandler* pH, core::XFile* pFile, bool keepEol = false);
    int readBytes(IFunctionHandler* pH, core::XFile* pFile, uint32_t numBytes);

protected:
    static int garbageCollect(IFunctionHandler* pH, void* pBuffer, int size);
    static core::XFile* getFile(IFunctionHandler* pH);

protected:
    core::IFileSys* pFileSys_;
};

class XBinds_Io : public XScriptBindsBase
{
public:
    XBinds_Io(XScriptSys* pSS);
    ~XBinds_Io();

    void bind(ICore* pCore) X_FINAL;

private:
    int openFile(IFunctionHandler* pH);
    int closeFile(IFunctionHandler* pH);

    SmartScriptTable WrapFileReturn(core::XFile* pFile);

private:
    core::IFileSys* pFileSys_;

    XBinds_Io_File file_;
};

X_NAMESPACE_END
