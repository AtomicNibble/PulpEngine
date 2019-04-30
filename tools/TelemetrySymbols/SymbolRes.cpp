#include "stdafx.h"
#include "SymbolRes.h"

#include <Containers/FixedArray.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(telemetry)

namespace
{
    const WORD DOS_HEADER_MAGIC = IMAGE_DOS_SIGNATURE;
    const WORD OPTIONAL_HEADER_MAGIC = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    const WORD OPTIONAL_HEADER_MAGIC64 = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    const DWORD NT_SIGNATURE = IMAGE_NT_SIGNATURE;

    const WORD IMAGE_FILE_I386 = IMAGE_FILE_MACHINE_I386;
    const WORD IMAGE_FILE_IA64 = IMAGE_FILE_MACHINE_IA64;
    const WORD IMAGE_FILE_AMD64 = IMAGE_FILE_MACHINE_AMD64;

    const size_t NUM_DATA_DIRECTORYS = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    struct MyNtHeader;
    struct MyOptionalHeader;

    struct PdbInfo
    {
        DWORD     Signature;
        BYTE      Guid[16];
        DWORD     Age;
        char      PdbFileName[1];
    };

#if X_64

    struct MyDosHeader : public IMAGE_DOS_HEADER
    {
        bool isValid(void) const {
            return this->e_magic == DOS_HEADER_MAGIC;
        }

        MyNtHeader* GetNtHeader(void) const {
            return reinterpret_cast<MyNtHeader*>(e_lfanew + reinterpret_cast<size_t>(this));
        }
    };

    struct MyNtHeader : public IMAGE_NT_HEADERS64
    {
        bool isValid(void) const {
            return this->Signature == NT_SIGNATURE;
        }
        MyOptionalHeader* GetOptionalHeader(void) {
            return reinterpret_cast<MyOptionalHeader*>(&OptionalHeader);
        }
    };

    struct MyOptionalHeader : public IMAGE_OPTIONAL_HEADER64
    {
        bool isValid(void) const {
            return this->Magic == OPTIONAL_HEADER_MAGIC64;
        }
    };

#else

    struct MyDosHeader : public IMAGE_DOS_HEADER
    {
        bool isValid(void) const {
            return this->e_magic == DOS_HEADER_MAGIC;
        }
        MyNtHeader* GetNtHeader(void) const {
            return reinterpret_cast<MyNtHeader*>(e_lfanew + reinterpret_cast<LONG>(this));
        }
    };

    struct MyNtHeader : public IMAGE_NT_HEADERS32
    {
        bool isValid(void) const {
            return this->Signature == NT_SIGNATURE;
        }
        MyOptionalHeader* GetOptionalHeader(void) {
            return reinterpret_cast<MyOptionalHeader*>(&OptionalHeader);
        }
    };

    struct MyOptionalHeader : public IMAGE_OPTIONAL_HEADER32
    {
        bool isValid(void) const {
            return this->Magic == OPTIONAL_HEADER_MAGIC;
        }

        bool is64Bit(void) const {
            return this->Magic == OPTIONAL_HEADER_MAGIC64;
        }

    };
#endif // X_64

    // is there a max I forget..
    using FixedSectionArray = core::FixedArray<IMAGE_SECTION_HEADER, 64>;

    uint32_t getFileOffsetForVA(const FixedSectionArray& sections, uint32_t rva)
    {
        for (auto& section : sections)
        {
            auto start = section.VirtualAddress;
            auto end = start + section.Misc.VirtualSize;

            if (rva >= start && rva < end) {

                // turn this into file offset.
                auto sectionRVA = rva - section.VirtualAddress;
                auto fileOffset = section.PointerToRawData + sectionRVA;

                return fileOffset;
            }
        }

        X_WARNING("TelemSym", "Failed to map RVA %" PRIx32 " to file offset", rva);
        return rva;
    }

    bool isMatchingPE(core::XFile* pFile, const SymModule::GuidByteArr& guid)
    {
        // TODO: be fancy and read in 4k pages from disk so all the header loading is only one disk op.
        MyDosHeader dosHdr;
        MyNtHeader ntHdr;

        if(pFile->readObj(dosHdr) != sizeof(dosHdr)) {
            return false;
        }
        if (!dosHdr.isValid()) {
            return false;
        }

        auto offset = sizeof(dosHdr) + dosHdr.e_lfanew;
        pFile->seek(offset, core::SeekMode::SET);

        if (pFile->readObj(ntHdr) != sizeof(ntHdr)) {
            return false;
        }
        if (!ntHdr.isValid()) {
            return false;
        }

        auto& optHdr = reinterpret_cast<const MyOptionalHeader&>(ntHdr.OptionalHeader);
        if (!optHdr.isValid()) {
            return false;
        }

        // load the sections.
        offset = offset + sizeof(ntHdr.Signature) + sizeof(ntHdr.FileHeader) + ntHdr.FileHeader.SizeOfOptionalHeader;
        pFile->seek(offset, core::SeekMode::SET);

        FixedSectionArray sections;

        size_t numSections = static_cast<size_t>(ntHdr.FileHeader.NumberOfSections);
        if (numSections > sections.capacity()) {
            X_WARNING("TelemSym", "PE has %" PRIuS " sections max %" PRIuS, numSections, sections.size());
            numSections = sections.capacity();
        }

        sections.resize(numSections);
        if (pFile->readObj(sections.data(), sections.size()) != sizeof(FixedSectionArray::Type) * sections.size()) {
            return false;
        }

        const IMAGE_DATA_DIRECTORY& dir = optHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        
        offset = getFileOffsetForVA(sections, dir.VirtualAddress);
        pFile->seek(offset, core::SeekMode::SET);

        IMAGE_DEBUG_DIRECTORY debugDir;
        if (pFile->readObj(debugDir) != sizeof(debugDir)) {
            return false;
        }

        if (debugDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
            return false;
        }

        offset = getFileOffsetForVA(sections, debugDir.AddressOfRawData);
        pFile->seek(offset, core::SeekMode::SET);

        PdbInfo pdbInfo;
        if (pFile->readObj(pdbInfo) != sizeof(pdbInfo)) {
            return false;
        }

        if (memcmp(&pdbInfo.Signature, "RSDS", 4) != 0) {
            return false;
        }

        static_assert(sizeof(pdbInfo.Guid) == sizeof(guid), "Size mismatch");

        if(memcmp(pdbInfo.Guid, guid.data(), sizeof(guid)) != 0) {
            return false;
        }

        // it's a match!
        // TODO: look at age?
        return true;
    }

} // namepace

SymResolver::~SymResolver()
{
    if (pSource_) {
        pSource_->Release();
    }
    if (pSession_) {
        pSession_->Release();
    }
}

bool SymResolver::init(void)
{
    auto hr = CoCreateInstance(
        CLSID_DiaSource,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDiaDataSource,
        (void **)&pSource_
    );

    if (FAILED(hr)) {
        X_ERROR("TelemSym", "Failed to create DIA data source. Error: 0x%" PRIu32, hr);
        return false;
    }

    hr = pSource_->openSession(&pSession_);
    if (FAILED(hr)) {
        X_ERROR("TelemSym", "Failed to open DIA session. Error: 0x%" PRIu32, hr);
        return false;
    }

    // sweet..
    return true;
}

bool SymResolver::loadPDB(uintptr_t baseAddr, uint32_t virtualSize, const GuidByteArr& guid, core::string_view path)
{
    if (paths_.isEmpty()) {
        X_ERROR("TelemSym", "Can't load PDB not symbol paths defined");
        return false;
    }

    if (haveModuleWithBase(baseAddr)) {
        X_ERROR("TelemSym", "Can't load PDB a modules with same base addr is already loaded");
        return false;
    }

    SymModule mod;
    mod.baseAddr_ = baseAddr;
    mod.virtualSize_ = virtualSize;
    mod.guid_ = guid;
    mod.path_.assign(path.begin(), path.length());

    // right now we need to try and actually find the PDB.
    // we search all the dir and check the guid of the PE.



    return true;
}


bool SymResolver::haveModuleWithBase(uintptr_t baseAddr)
{
    return std::find_if(modules_.begin(), modules_.end(), [baseAddr](const SymModuleArr::Type& mod) {
        return mod.baseAddr_ == baseAddr;
    }) != modules_.end();
}

X_NAMESPACE_END
