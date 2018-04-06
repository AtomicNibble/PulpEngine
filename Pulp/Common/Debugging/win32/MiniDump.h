#pragma once

#ifndef X_MINIDUMP_H
#define X_MINIDUMP_H

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
namespace debugging
{
    /// \brief Writes a minidump to a file.
    /// \details A dump file allows programmers to save program information for debugging at a later time. They are
    /// especially useful when running a program on any computer that does not have the source code, symbol files, or
    /// a debugger installed.
    ///
    /// Minidumps written by this function do not contain any heap information, hence the application binary as well as
    /// the .pdb symbol files must be available when debugging a dump file using Visual Studio. Not storing heap information
    /// creates minidumps which are much smaller compared to a full dump file.
    /// See http://msdn.microsoft.com/en-us/library/windows/desktop/ee416349%28v=vs.85%29.aspx for more details.
    /// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_MINI_DUMP. Disabling
    /// writing of mini dumps reduces the executable's size, and may be useful for some builds.
    /// \sa X_ENABLE_MINI_DUMP exceptionHandler

    enum class DumpType
    {
        Full,
        Medium,
        Small
    };

    bool WriteMiniDump(const Path<char>& filename, DumpType type, EXCEPTION_POINTERS* exceptionPointers);
    bool WriteMiniDump(const Path<wchar_t>& filename, DumpType type, EXCEPTION_POINTERS* exceptionPointers);
} // namespace debugging

X_NAMESPACE_END

#endif
