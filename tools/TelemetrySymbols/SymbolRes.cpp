#include "stdafx.h"
#include "SymbolRes.h"

#include <Containers/FixedArray.h>
#include <Containers/FixedByteStream.h>

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
            return e_magic == DOS_HEADER_MAGIC;
        }

        MyNtHeader* GetNtHeader(void) const {
            return reinterpret_cast<MyNtHeader*>(e_lfanew + reinterpret_cast<size_t>(this));
        }
    };

    struct MyNtHeader : public IMAGE_NT_HEADERS64
    {
        bool isValid(void) const {
            return Signature == NT_SIGNATURE;
        }
        MyOptionalHeader* GetOptionalHeader(void) {
            return reinterpret_cast<MyOptionalHeader*>(&OptionalHeader);
        }
    };

    struct MyOptionalHeader : public IMAGE_OPTIONAL_HEADER64
    {
        bool isValid(void) const {
            return Magic == OPTIONAL_HEADER_MAGIC64;
        }
    };

#else // X_64

    struct MyDosHeader : public IMAGE_DOS_HEADER
    {
        bool isValid(void) const {
            return e_magic == DOS_HEADER_MAGIC;
        }
        MyNtHeader* GetNtHeader(void) const {
            return reinterpret_cast<MyNtHeader*>(e_lfanew + reinterpret_cast<LONG>(this));
        }
    };

    struct MyNtHeader : public IMAGE_NT_HEADERS32
    {
        bool isValid(void) const {
            return Signature == NT_SIGNATURE;
        }
        MyOptionalHeader* GetOptionalHeader(void) {
            return reinterpret_cast<MyOptionalHeader*>(&OptionalHeader);
        }
    };

    struct MyOptionalHeader : public IMAGE_OPTIONAL_HEADER32
    {
        bool isValid(void) const {
            return Magic == OPTIONAL_HEADER_MAGIC;
        }

        bool is64Bit(void) const {
            return Magic == OPTIONAL_HEADER_MAGIC64;
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

    bool isMatchingPE(core::XFile* pFile, const core::Guid& guid)
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

        if(memcmp(pdbInfo.Guid, guid.bytes().data(), sizeof(guid)) != 0) {
            return false;
        }

        // it's a match!
        // TODO: look at age?
        return true;
    }

} // namepace

SymModule::SymModule() :
    baseAddr_(0),
    virtualSize_(0),
    age_(0),
    pSource_(nullptr),
    pSession_(nullptr)
{
}

SymModule::SymModule(SymModule&& oth) :
    baseAddr_(oth.baseAddr_),
    virtualSize_(oth.virtualSize_),
    guid_(oth.guid_),
    age_(oth.age_),
    path_(oth.path_),
    pSource_(oth.pSource_),
    pSession_(oth.pSession_)
{
    oth.pSource_ = nullptr;
    oth.pSession_ = nullptr;
}

SymModule::~SymModule()
{
    if (pSession_) {
        pSession_->Release();
    }
    if (pSource_) {
        pSource_->Release();
    }

}

SymModule& SymModule::operator=(SymModule&& oth)
{
    baseAddr_ = oth.baseAddr_;
    virtualSize_ = oth.virtualSize_;
    guid_ = oth.guid_;
    age_ = oth.age_;
    path_ = oth.path_;
    pSource_ = oth.pSource_;
    pSession_ = oth.pSession_;
    
    oth.pSource_ = nullptr;
    oth.pSession_ = nullptr;
    return *this;
}


// ---------------------------------------

SymResolver::SymResolver(core::MemoryArenaBase* arena) :
    searchPaths_(arena),
    modules_(arena),
    comInit_(false)
{
}


SymResolver::~SymResolver()
{
    modules_.clear(); // cleanup before we un init com.

    if (comInit_) {
        ::CoUninitialize();
    }
}

bool SymResolver::init(void)
{
    auto hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hr != S_OK && hr != S_FALSE) {
        X_ERROR("TelemSym", "Failed to create DIA data source. Error: 0x%" PRIu32, hr);
        return false;
    }

    comInit_ = true;
    return true;
}

bool SymResolver::loadPDB(uintptr_t baseAddr, uint32_t virtualSize, const core::Guid& guid, uint32_t age, core::string_view path)
{
    if (searchPaths_.isEmpty()) {
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
    mod.age_ = age;
    mod.path_.assign(path.begin(), path.length());

    core::Path<> tmp(path.begin(), path.end());
    tmp.replaceSeprators();


    core::Path<> symbolPath;
    symbolPath.append(tmp.fileName());
    addSymSrvFolderNameForPDB(symbolPath, guid, age);
    symbolPath.append(tmp.fileName());

    core::Path<> fullPath;
    for (auto& searchPath : searchPaths_)
    {
        if (searchPath.type == SymPathType::Path || searchPath.type == SymPathType::Cache)
        {
            fullPath = searchPath.path;
            fullPath /= symbolPath;

            if (gEnv->pFileSys->fileExistsOS(fullPath))
            {
                break;
            }
            else
            {
                fullPath.clear();
            }
        }
    }

    if (fullPath.isEmpty()) {
        // Failed to find it.
        return false;
    }

    // Load the PDB.
    auto hr = CoCreateInstance(
        CLSID_DiaSource,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDiaDataSource,
        (void **)&mod.pSource_
    );

    if (FAILED(hr)) {
        X_ERROR("TelemSym", "Failed to create DIA data source. Error: 0x%" PRIu32, hr);
        return false;
    }

    core::Path<wchar_t> fullPathW(fullPath);

    hr = mod.pSource_->loadDataFromPdb(fullPathW.c_str());
    if (FAILED(hr)) {
        X_ERROR("TelemSym", "loadDataFromPdb failed. Error: 0x%" PRIu32, hr);
        return false;
    }

    hr = mod.pSource_->openSession(&mod.pSession_);
    if (FAILED(hr)) {
        X_ERROR("TelemSym", "Failed to open DIA session. Error: 0x%" PRIu32, hr);
        return false;
    }

    modules_.emplace_back(std::move(mod));
    return true;
}

void SymResolver::setCachePath(core::string_view path)
{
    SymPath sp;
    sp.path.set(path.begin(), path.length());
    sp.path.replaceSeprators();
    sp.type = SymPathType::Cache;

    // Make sure the cache is always at front.
    if (searchPaths_.isNotEmpty()) {
        auto& firstSp = searchPaths_.front();
        if (firstSp.type == SymPathType::Cache) {
            firstSp = sp;
            return;
        }
    }

    searchPaths_.append(sp);
}

void SymResolver::addPath(core::string_view path, SymPathType::Enum type)
{
    // TODO: check unique.
    SymPath sp;
    sp.path.set(path.begin(), path.length());
    sp.path.replaceSeprators();
    sp.type = type;

    for (auto& searchPath : searchPaths_)
    {
        // TODO: do a proper OS level check?
        if (searchPath.path == sp.path)
        {
            X_ERROR("TelemSym", "Duplicate symbol path. \"%s\"", sp.path.c_str());
            return;
        }
    }

    searchPaths_.append(sp);
}

bool SymResolver::resolveForAddr(uintptr_t addr)
{
    for (auto& mod : modules_)
    {
        if (mod.containsAddr(addr))
        {
            auto rva = static_cast<uint32_t>((addr - mod.baseAddr_) & 0xFFFFFFFF); // 0x1EE30;

            IDiaSymbol* pFunc;
            LONG disp = 0;
            mod.pSession_->findSymbolByRVAEx(rva, SymTagFunction, &pFunc, &disp);

            BSTR pName = nullptr;
            pFunc->get_name(&pName);

            return true;
        }
    }

    return false;
}

void SymResolver::addSymSrvFolderNameForPDB(core::Path<>& path, const core::Guid& guid, uint32_t age)
{
    auto& g = guid.guid();

    path.ensureSlash();
    path.appendFmt("%.8" PRIX32, g.data1);
    path.appendFmt("%.4" PRIX16, g.data2);
    path.appendFmt("%.4" PRIX16, g.data3);
    for (auto byte : g.data4) {
        path.appendFmt("%.2" PRIX8, byte);
    }
    path.appendFmt("%" PRIX32, age);
    path.ensureSlash();
}

bool SymResolver::getSymInfoFromPDB(core::Path<>& path, SymInfo& info)
{
    std::array<uint8_t, 0x20> PDB7_SIG = {
        0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x43, 0x2F, 0x43, 0x2B, 0x2B, 0x20,
        0x4D, 0x53, 0x46, 0x20, 0x37, 0x2E, 0x30, 0x30, 0x0D, 0x0A, 0x1A, 0x44, 0x53, 0x00, 0x00, 0x00
    };

    struct PDB_SIGNATURE
    {
        std::array<uint8_t, 0x20> sig;
    };

#if 0
    struct PDB_HEADER2
    {
        PDB_SIGNATURE m_signature;
        uint32_t pageSize;
        uint32_t startPage;
        uint32_t filePages;
        uint32_t rootStreamSize;
        uint32_t reserved;
        uint32_t rootPageIndex;
    };
#endif

    struct PDB_HEADER7
    {
        inline size_t pageCount(size_t size) const {
            return (size + pageSize - 1) / pageSize;
        }
        inline size_t offsetforPageIdx(size_t idx) const {
            return idx * pageSize;
        }
        inline size_t rootPageCount(void) const {
            return pageCount(rootStreamSize);
        }
        inline size_t rootPageOffset(void) const {
            return offsetforPageIdx(rootPageIndex);
        }

        PDB_SIGNATURE signature;
        uint32_t pageSize;
        uint32_t AllocTablePointer;
        uint32_t filePages;
        uint32_t rootStreamSize;
        uint32_t reserved;
        uint32_t rootPageIndex;
    };

    core::XFileScoped file;
    if (!file.openFileOS(path, core::FileFlag::READ | core::FileFlag::SHARE | core::FileFlag::RANDOM_ACCESS)) {
        return false;
    }

    PDB_HEADER7 hdr;
    if (file.readObj(hdr) != sizeof(hdr)) {
        X_ERROR("TelemSym", "Failed to read PDB header");
        return false;
    }

    if (hdr.signature.sig != PDB7_SIG) {
        X_ERROR("TelemSym", "PDB sig mismatch");
        return false;
    }

    auto rootPageCount = hdr.rootPageCount();
    core::Array<uint32_t> rootPages(g_TelemSymLibArena, rootPageCount);

    auto rootRageOffset = hdr.rootPageOffset();
    file.seek(rootRageOffset, core::SeekMode::SET);

    if (file.readObjs(rootPages.data(), rootPageCount) != rootPageCount) {
        X_ERROR("TelemSym", "Failed to read root page indexes");
        return false;
    }

    // we want to read info from root streams, for now just read it all.
    core::Array<uint8_t> buff(g_TelemSymLibArena, hdr.rootStreamSize);

    // read the data.
    {
        for (int32_t i = 0; i < static_cast<int32_t>(rootPages.size()); i++)
        {
            uint32_t offset = i * hdr.pageSize;
            uint32_t bytesLeft = hdr.rootStreamSize - offset;

            uint32_t pageIdx = rootPages[i];
            uint32_t readSize = core::Min(hdr.pageSize, bytesLeft);
            auto pageOffset = hdr.offsetforPageIdx(pageIdx);

            file.seek(pageOffset, core::SeekMode::SET);

            if (file.read(buff.data() + offset, readSize) != readSize) {
                X_ERROR("TelemSym", "Failed to read root page data");
                return false;
            }
        }
    }

    core::FixedByteStreamNoneOwning bs(buff.begin(), buff.end(), true);

    auto numStreams = bs.read<uint32_t>();
    if (numStreams < 2) {
        X_ERROR("TelemSym", "PDB is missing stream1");
        return false;
    }

    auto stream0Size = bs.read<uint32_t>();
    auto stream0Pages = hdr.pageCount(stream0Size);

    auto stream1Size = bs.read<uint32_t>();
    auto stream1Pages = hdr.pageCount(stream1Size);
    X_UNUSED(stream1Pages);

    bs.skip(sizeof(uint32_t) * (numStreams - 2));

    // skip steam0 pages
    bs.skip(sizeof(uint32_t) * stream0Pages);

    struct PDBVersionInfo
    {
        uint32_t version;
        uint32_t timeStamp;
        uint32_t age;
        core::Guid guid;
        uint32_t stringDataLength;
    };

    // Make this bit more simple and only care about first page
    // think it's very unlikely page size ever be less than 32bytes.
    // be surprised if it was ever not 4096.
    if (sizeof(PDBVersionInfo) > hdr.pageSize) {
        X_ERROR("TelemSym", "Unsupported PDB pagesize %" PRIu32, hdr.pageSize);
        return false;
    }

    auto pageIdx = bs.read<uint32_t>();
    auto pageOffset = hdr.offsetforPageIdx(pageIdx);

    file.seek(pageOffset, core::SeekMode::SET);

    PDBVersionInfo pdbInfo;
    if (file.readObj(pdbInfo) != sizeof(pdbInfo)) {
        X_ERROR("TelemSym", "Failed to read PDB version info");
        return false;
    }

    info.age = pdbInfo.age;
    info.guid = pdbInfo.guid;
    return true;
}

bool SymResolver::haveModuleWithBase(uintptr_t baseAddr)
{
    return std::find_if(modules_.begin(), modules_.end(), [baseAddr](const SymModuleArr::Type& mod) {
        return mod.baseAddr_ == baseAddr;
    }) != modules_.end();
}


X_NAMESPACE_END
