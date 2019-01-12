#include "EngineCommon.h"

X_DISABLE_WARNING(4091)
#include <DbgHelp.h>
X_ENABLE_WARNING(4091)

#include "ExceptionHandler.h"

// #include "Debugging\SymbolInfo.h"
#include "SymbolResolution.h"

#include "MiniDump.h"

#include <Time\DateStamp.h>
#include <Time\TimeStamp.h>
#include <Platform\MessageBox.h>
#include <Platform\SystemInfo.h>
#include <String\HumanSize.h>

#if X_ENABLE_UNHANDLED_EXCEPTION_HANDLER

X_LINK_LIB("dbghelp.lib")

#endif

X_NAMESPACE_BEGIN(core)

using namespace string_view_literals;

namespace exceptionHandler
{
    namespace
    {
        LPTOP_LEVEL_EXCEPTION_FILTER g_oldExceptionFilter;

        const char* GetExceptionName(DWORD code)
        {
            if (code > EXCEPTION_ACCESS_VIOLATION) {
#define ADD_VAL_STR(val) \
    case val:            \
        return #val;

                switch (code) {
                    ADD_VAL_STR(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
                    ADD_VAL_STR(EXCEPTION_FLT_DENORMAL_OPERAND)
                    ADD_VAL_STR(EXCEPTION_FLT_DIVIDE_BY_ZERO)
                    ADD_VAL_STR(EXCEPTION_FLT_INEXACT_RESULT)
                    ADD_VAL_STR(EXCEPTION_FLT_INVALID_OPERATION)
                    ADD_VAL_STR(EXCEPTION_FLT_OVERFLOW)
                    ADD_VAL_STR(EXCEPTION_FLT_STACK_CHECK)
                    ADD_VAL_STR(EXCEPTION_FLT_UNDERFLOW)
                    ADD_VAL_STR(EXCEPTION_PRIV_INSTRUCTION)
                    ADD_VAL_STR(EXCEPTION_IN_PAGE_ERROR)
                    ADD_VAL_STR(EXCEPTION_INT_DIVIDE_BY_ZERO)
                    ADD_VAL_STR(EXCEPTION_INT_OVERFLOW)
                    ADD_VAL_STR(EXCEPTION_INVALID_DISPOSITION)
                    ADD_VAL_STR(EXCEPTION_NONCONTINUABLE_EXCEPTION)
                    ADD_VAL_STR(EXCEPTION_SINGLE_STEP)
                    ADD_VAL_STR(EXCEPTION_STACK_OVERFLOW)
                    default:
                        break;
                }

#undef ADD_VAL_STR
            }
            else {
                if (code == EXCEPTION_ACCESS_VIOLATION) {
                    return "EXCEPTION_ACCESS_VIOLATION";
                }
                else {
                    if (code > 0x8FFFFF00) {
                        if (code == 0x8FFFFF01)
                            return "EXCEPTION_PURE_VIRTUAL_FUNCTION_CALLED";
                        if (code == 0x8FFFFF02)
                            return "EXCEPTION_CRT_INVALID_PARAMETER_CALL";
                    }
                    else {
                        if (code == 0x8FFFFF00)
                            return "EXCEPTION_ABORT_CALLED";
                        if (code == 0x80000002)
                            return "EXCEPTION_DATATYPE_MISALIGNMENT";
                        if (code == 0x80000001)
                            return "EXCEPTION_BREAKPOINT";
                        if (code == 0x80000000)
                            return "EXCEPTION_SINGLE_STEP";
                    }
                }
            }

            return "Unrecognized exception.";
        }

        const char* GetExceptionDescription(DWORD code)
        {
            const char* str = "Unrecognized exception.";

            if (code > EXCEPTION_ACCESS_VIOLATION) {
                switch (code) {
                    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                        str = "The thread tried to access an array element that is out of bounds.";
                        break;
                    case EXCEPTION_FLT_DENORMAL_OPERAND:
                        str = "One of the operands in a floating-point operation is denormal.";
                        break;
                    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                        str = "The thread tried to divide a floating-point value by a floating-point divisor of zero.";
                        break;
                    case EXCEPTION_FLT_INEXACT_RESULT:
                        str = "The str of a floating-point operation cannot be represented exactly as a decimal fraction.";
                        break;
                    case EXCEPTION_FLT_INVALID_OPERATION:
                        str = "The thread tried to execute an invalid floating-point operation.";
                        break;
                    case EXCEPTION_FLT_OVERFLOW:
                        str = "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
                        break;
                    case EXCEPTION_FLT_STACK_CHECK:
                        str = "The stack overflowed or underflowed as the str of a floating-point operation.";
                        break;
                    case EXCEPTION_FLT_UNDERFLOW:
                        str = "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
                        break;
                    case EXCEPTION_PRIV_INSTRUCTION:
                        str = "The thread tried to execute an invalid instruction.";
                        break;
                    case EXCEPTION_IN_PAGE_ERROR:
                        str = "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.";
                        break;
                    case EXCEPTION_INT_DIVIDE_BY_ZERO:
                        str = "The thread tried to divide an integer value by an integer divisor of zero.";
                        break;
                    case EXCEPTION_INT_OVERFLOW:
                        str = "The str of an integer operation caused a carry out of the most significant bit of the str.";
                        break;
                    case EXCEPTION_INVALID_DISPOSITION:
                        str = "An exception handler returned an invalid disposition to the exception dispatcher.";
                        break;
                    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                        str = "The thread tried to continue execution after a noncontinuable exception occurred.";
                        break;
                    case EXCEPTION_SINGLE_STEP:
                        str = "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
                        break;
                    case EXCEPTION_STACK_OVERFLOW:
                        str = "The thread used up its stack.";
                        break;
                    default:
                        break;
                }
            }
            else {
                if (code == EXCEPTION_ACCESS_VIOLATION) {
                    str = "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
                }
                else {
                    if (code > 0x8FFFFF00) {
                        if (code == 0x8FFFFF01)
                            return "A pure virtual function has been called.";
                        if (code == 0x8FFFFF02)
                            return "A CRT function has been called with an invalid parameter.";
                    }
                    else {
                        if (code == 0x8FFFFF00)
                            return "abort() has been called.";
                        if (code == 0x80000002)
                            return "The thread tried to read or write data that is misaligned.";
                        if (code == 0x80000001)
                            return "A breakpoint was encountered.";
                        if (code == 0x80000000)
                            return "A trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
                    }
                }
            }

            return str;
        }

    } // namespace

#if X_ENABLE_UNHANDLED_EXCEPTION_HANDLER

    namespace internal
    {
        LONG WINAPI HandleException(_EXCEPTION_POINTERS* exceptionPointers)
        {
            PEXCEPTION_RECORD ExceptionRecord = exceptionPointers->ExceptionRecord;
            PCONTEXT ContextRecord = exceptionPointers->ContextRecord;

            DWORD Code = ExceptionRecord->ExceptionCode;
            const bool isNonContinuable = ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE;

            X_ERROR("ExceptionHandler", "%s exception %s (0x%08X) occurred at address: 0x%016" PRIxPTR,
                (isNonContinuable ? "None-Continuable" : "Continuable"),
                GetExceptionName(Code),
                exceptionPointers,
                ExceptionRecord->ExceptionAddress);

            {
                X_LOG_BULLET;
                X_ERROR("ExceptionHandler", "Reason: %s", GetExceptionDescription(Code));

                if ((Code == STATUS_ACCESS_VIOLATION || Code == STATUS_IN_PAGE_ERROR) && ExceptionRecord->NumberParameters) {
                    ULONG_PTR ExpInfo = ExceptionRecord->ExceptionInformation[0];
                    if (ExpInfo) {
                        if (ExpInfo == 1) {
                            X_ERROR("ExceptionHandler", "Information: The thread attempted to write to an inaccessible address at virtual address: 0x%016" PRIxPTR, ExceptionRecord->ExceptionAddress);
                        }
                        else if (ExpInfo == 8) {
                            X_ERROR("ExceptionHandler", "Information: The thread caused a user-mode data execution prevention (DEP) violation at virtual address: 0x%016" PRIxPTR, ExceptionRecord->ExceptionAddress);
                        }
                    }
                    else {
                        X_ERROR("ExceptionHandler", "Information: The thread attempted to read the inaccessible data at virtual address: 0x%016" PRIdPTR, ExceptionRecord->ExceptionAddress);
                    }
                }

                X_LOG0("ExceptionHandler", "Callstack:");
                {
                    X_LOG_BULLET;

                    _tagSTACKFRAME64 stackFrame;
                    core::zero_object(stackFrame);

#if X_64 == 1
                    stackFrame.AddrPC.Offset = ContextRecord->Rip;
                    stackFrame.AddrPC.Mode = AddrModeFlat;
                    stackFrame.AddrFrame.Offset = ContextRecord->Rbp;
                    stackFrame.AddrFrame.Mode = AddrModeFlat;
                    stackFrame.AddrStack.Offset = ContextRecord->Rsp;
                    stackFrame.AddrStack.Mode = AddrModeFlat;

                    const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
#else
                    stackFrame.AddrPC.Offset = ContextRecord->Eip;
                    stackFrame.AddrPC.Mode = AddrModeFlat;
                    stackFrame.AddrFrame.Offset = ContextRecord->Ebp;
                    stackFrame.AddrFrame.Mode = AddrModeFlat;
                    stackFrame.AddrStack.Offset = ContextRecord->Esp;
                    stackFrame.AddrStack.Mode = AddrModeFlat;

                    const DWORD machine = IMAGE_FILE_MACHINE_I386;

#endif // !X_64

                    HANDLE CurrentThread = GetCurrentThread();
                    HANDLE CurrentPro = GetCurrentProcess();

                    if (StackWalk64(machine, CurrentPro, CurrentThread, &stackFrame, ContextRecord, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) {
                        do {
                            if (!stackFrame.AddrPC.Offset)
                                break;
                            if (!stackFrame.AddrReturn.Offset)
                                break;

                            SymbolInfo info = symbolResolution::ResolveSymbolsForAddress((void*)stackFrame.AddrPC.Offset);

                            X_LOG0("ExceptionHandler", "%s(%" PRIu32 "): %s (0x%016" PRIx64 ")",
                                info.GetFilename(), info.GetLine(), info.GetFunction(), stackFrame.AddrReturn.Offset);

                        } while (StackWalk64(machine, CurrentPro, CurrentThread, &stackFrame, ContextRecord, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0));
                    }
                    else {
                        core::lastError::Description Dsc;
                        X_ERROR("ExceptionHandler", "Failed to walk stack: \"%s\"", core::lastError::ToString(Dsc));
                    }

                } // end of call stack

                X_LOG0("ExceptionHandler", "Machine context:");

                if (ContextRecord->ContextFlags & CONTEXT_CONTROL) // AMD64 | INT
                {
                    X_LOG0("ExceptionHandler", "CPU integer registers:");
                    {
                        X_LOG_BULLET;

#if X_64 == 1
                        X_LOG0("ExceptionHandler", "RAX = 0x%016" PRIxPTR, ContextRecord->Rax);
                        X_LOG0("ExceptionHandler", "RCX = 0x%016" PRIxPTR, ContextRecord->Rcx);
                        X_LOG0("ExceptionHandler", "RDX = 0x%016" PRIxPTR, ContextRecord->Rdx);
                        X_LOG0("ExceptionHandler", "RBX = 0x%016" PRIxPTR, ContextRecord->Rbx);
                        X_LOG0("ExceptionHandler", "RSI = 0x%016" PRIxPTR, ContextRecord->Rsi);
                        X_LOG0("ExceptionHandler", "RDI = 0x%016" PRIxPTR, ContextRecord->Rdi);
                        X_LOG0("ExceptionHandler", "R08 = 0x%016" PRIxPTR, ContextRecord->R8);
                        X_LOG0("ExceptionHandler", "R09 = 0x%016" PRIxPTR, ContextRecord->R9);
                        X_LOG0("ExceptionHandler", "R10 = 0x%016" PRIxPTR, ContextRecord->R10);
                        X_LOG0("ExceptionHandler", "R11 = 0x%016" PRIxPTR, ContextRecord->R11);
                        X_LOG0("ExceptionHandler", "R12 = 0x%016" PRIxPTR, ContextRecord->R12);
                        X_LOG0("ExceptionHandler", "R13 = 0x%016" PRIxPTR, ContextRecord->R13);
                        X_LOG0("ExceptionHandler", "R14 = 0x%016" PRIxPTR, ContextRecord->R14);
                        X_LOG0("ExceptionHandler", "R15 = 0x%016" PRIxPTR, ContextRecord->R15);
#else
                        X_LOG0("ExceptionHandler", "EAX = 0x%08" PRIxPTR, ContextRecord->Eax);
                        X_LOG0("ExceptionHandler", "EBX = 0x%08" PRIxPTR, ContextRecord->Ebx);
                        X_LOG0("ExceptionHandler", "ECX = 0x%08" PRIxPTR, ContextRecord->Ecx);
                        X_LOG0("ExceptionHandler", "EDX = 0x%08" PRIxPTR, ContextRecord->Edx);
                        X_LOG0("ExceptionHandler", "ESI = 0x%08" PRIxPTR, ContextRecord->Esi);
                        X_LOG0("ExceptionHandler", "EDI = 0x%08" PRIxPTR, ContextRecord->Edi);
#endif // !X_64
                    }
                }

                if (ContextRecord->ContextFlags & CONTEXT_SEGMENTS) {
                    X_LOG0("ExceptionHandler", "CPU segments:");
                    {
                        X_LOG_BULLET;

                        X_LOG0("ExceptionHandler", "DS = 0x%04" PRIx32, ContextRecord->SegDs);
                        X_LOG0("ExceptionHandler", "ES = 0x%04" PRIx32, ContextRecord->SegEs);
                        X_LOG0("ExceptionHandler", "FS = 0x%04" PRIx32, ContextRecord->SegFs);
                        X_LOG0("ExceptionHandler", "GS = 0x%04" PRIx32, ContextRecord->SegGs);
                    }
                }

                if (ContextRecord->ContextFlags & CONTEXT_CONTROL) {
                    X_LOG0("ExceptionHandler", "CPU control registers:");
                    {
                        X_LOG_BULLET;

#if X_64 == 1
                        X_LOG0("ExceptionHandler", "RIP	  = 0x%016" PRIxPTR, ContextRecord->Rip);
                        X_LOG0("ExceptionHandler", "RSP	  = 0x%016" PRIxPTR, ContextRecord->Rsp);
                        X_LOG0("ExceptionHandler", "RBP	  = 0x%016" PRIxPTR, ContextRecord->Rbp);
                        X_LOG0("ExceptionHandler", "EFlags = 0x%016" PRIxPTR, ContextRecord->EFlags);
                        X_LOG0("ExceptionHandler", "CS	  = 0x%016" PRIxPTR, ContextRecord->SegCs);
                        X_LOG0("ExceptionHandler", "SS	  = 0x%016" PRIxPTR, ContextRecord->SegSs);

#else

                        X_LOG0("ExceptionHandler", "EIP = 0x%08" PRIxPTR, ContextRecord->Eip);
                        X_LOG0("ExceptionHandler", "ESP = 0x%08" PRIxPTR, ContextRecord->Esp);
                        X_LOG0("ExceptionHandler", "EBP = 0x%08" PRIxPTR, ContextRecord->Ebp);
                        X_LOG0("ExceptionHandler", "EFL = 0x%08" PRIxPTR, ContextRecord->EFlags);
                        X_LOG0("ExceptionHandler", "CS = 0x%08" PRIxPTR, ContextRecord->SegCs);
                        X_LOG0("ExceptionHandler", "SS = 0x%08" PRIxPTR, ContextRecord->SegSs);

#endif // !X_64
                    }
                }

                if (ContextRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) {
                    X_LOG0("ExceptionHandler", "CPU debug registers:");
                    {
                        X_LOG_BULLET;

#if X_64 == 1
                        X_LOG0("ExceptionHandler", "Dr0 = 0x%016" PRIxPTR, ContextRecord->Dr0);
                        X_LOG0("ExceptionHandler", "Dr1 = 0x%016" PRIxPTR, ContextRecord->Dr1);
                        X_LOG0("ExceptionHandler", "Dr2 = 0x%016" PRIxPTR, ContextRecord->Dr2);
                        X_LOG0("ExceptionHandler", "Dr3 = 0x%016" PRIxPTR, ContextRecord->Dr3);
                        X_LOG0("ExceptionHandler", "Dr6 = 0x%016" PRIxPTR, ContextRecord->Dr6);
                        X_LOG0("ExceptionHandler", "Dr7 = 0x%016" PRIxPTR, ContextRecord->Dr7);
#else
                        X_LOG0("ExceptionHandler", "Dr0 = 0x%08" PRIxPTR, ContextRecord->Dr0);
                        X_LOG0("ExceptionHandler", "Dr1 = 0x%08" PRIxPTR, ContextRecord->Dr1);
                        X_LOG0("ExceptionHandler", "Dr2 = 0x%08" PRIxPTR, ContextRecord->Dr2);
                        X_LOG0("ExceptionHandler", "Dr3 = 0x%08" PRIxPTR, ContextRecord->Dr3);
                        X_LOG0("ExceptionHandler", "Dr6 = 0x%08" PRIxPTR, ContextRecord->Dr6);
                        X_LOG0("ExceptionHandler", "Dr7 = 0x%08" PRIxPTR, ContextRecord->Dr7);
#endif // !X_64
                    }
                }

#if X_64 == 1

                if (ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) {
                    X_LOG0("ExceptionHandler", "CPU floating-point state:");
                    {
                        X_LOG_BULLET;

                        X_LOG0("ExceptionHandler", "Xmm0 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm0.Low, ContextRecord->Xmm0.High);
                        X_LOG0("ExceptionHandler", "Xmm1 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm1.Low, ContextRecord->Xmm1.High);
                        X_LOG0("ExceptionHandler", "Xmm2 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm2.Low, ContextRecord->Xmm2.High);
                        X_LOG0("ExceptionHandler", "Xmm3 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm3.Low, ContextRecord->Xmm3.High);
                        X_LOG0("ExceptionHandler", "Xmm4 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm4.Low, ContextRecord->Xmm4.High);
                        X_LOG0("ExceptionHandler", "Xmm5 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm5.Low, ContextRecord->Xmm5.High);
                        X_LOG0("ExceptionHandler", "Xmm6 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm6.Low, ContextRecord->Xmm6.High);
                        X_LOG0("ExceptionHandler", "Xmm7 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm7.Low, ContextRecord->Xmm7.High);
                        X_LOG0("ExceptionHandler", "Xmm8 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm8.Low, ContextRecord->Xmm8.High);
                        X_LOG0("ExceptionHandler", "Xmm9 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm9.Low, ContextRecord->Xmm9.High);
                        X_LOG0("ExceptionHandler", "Xmm10 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm10.Low, ContextRecord->Xmm10.High);
                        X_LOG0("ExceptionHandler", "Xmm11 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm11.Low, ContextRecord->Xmm11.High);
                        X_LOG0("ExceptionHandler", "Xmm12 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm12.Low, ContextRecord->Xmm12.High);
                        X_LOG0("ExceptionHandler", "Xmm13 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm13.Low, ContextRecord->Xmm13.High);
                        X_LOG0("ExceptionHandler", "Xmm14 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm14.Low, ContextRecord->Xmm14.High);
                        X_LOG0("ExceptionHandler", "Xmm15 = 0x%016" PRIx64 "%016" PRIx64, ContextRecord->Xmm15.Low, ContextRecord->Xmm15.High);
                    }
                }

#else

                if (ContextRecord->ContextFlags & 0x10008) {
                    X_LOG0("ExceptionHandler", "CPU floating-point state:");
                    {
                        X_LOG_BULLET;

                        X_LOG0("ExceptionHandler", "ControlWord = 0x%08" PRIxPTR, ContextRecord->FloatSave.ControlWord);
                        X_LOG0("ExceptionHandler", "StatusWord = 0x%08" PRIxPTR, ContextRecord->FloatSave.StatusWord);
                        X_LOG0("ExceptionHandler", "TagWord = 0x%08" PRIxPTR, ContextRecord->FloatSave.TagWord);
                        X_LOG0("ExceptionHandler", "ErrorOffset = 0x%08" PRIxPTR, ContextRecord->FloatSave.ErrorOffset);
                        X_LOG0("ExceptionHandler", "ErrorSelector = 0x%08" PRIxPTR, ContextRecord->FloatSave.ErrorSelector);
                        X_LOG0("ExceptionHandler", "DataOffset = 0x%08" PRIxPTR, ContextRecord->FloatSave.DataOffset);
                        X_LOG0("ExceptionHandler", "DataSelector = 0x%08" PRIxPTR, ContextRecord->FloatSave.DataSelector);
                        X_LOG0("ExceptionHandler", "Spare0 = 0x%08" PRIxPTR, ContextRecord->FloatSave.Spare0); //Cr0NpxState );
                        //	X_LOG0("ExceptionHandler", "RegisterArea = 0x%08X", ContextRecord->FloatSave.RegisterArea); //Cr0NpxState );
                    }
                }
#endif // !X_64

                {
                    core::SysInfo::UserNameStr userName;
                    core::SysInfo::LanguageStr lang;
                    core::SysInfo::MemInfo memInfo;
                    core::SysInfo::DeviceMode mode;

                    core::SysInfo::GetUserName(userName);
                    core::SysInfo::GetLanguage(lang);
                    core::SysInfo::GetSystemMemInfo(memInfo);
                    core::SysInfo::GetCurrentDisplayMode(mode);

                    core::HumanSize::Str s1, s2, s3;

                    X_LOG0("ExceptionHandler", "UserName: \"%ls\"", userName);
                    X_LOG0("ExceptionHandler", "Language: \"%ls\"", lang);
                    X_LOG0("ExceptionHandler", "PhysicalMem %s available %s virtual %s used %ld%%",
                        core::HumanSize::toString(s1, memInfo.TotalPhys),
                        core::HumanSize::toString(s2, memInfo.AvailPhys),
                        core::HumanSize::toString(s3, memInfo.TotalVirtual),
                        memInfo.dwMemoryLoad);
                    X_LOG0("ExceptionHandler", "Display: %dx%dx%d",
                        mode.pelsWidth,
                        mode.pelsHeight,
                        mode.bitsPerPel);
                }

                {
                    TimeStamp time = TimeStamp::getSystemTime();
                    DateStamp date = DateStamp::getSystemDate();

                    TimeStamp::FileDescription time_txt;
                    DateStamp::Description date_txt;

                    Path<wchar_t> filename;
                    Path<wchar_t> modPath;

                    {
                        wchar_t buf[core::Path<>::BUF_SIZE] = {};
                        auto pathLength = ::GetModuleFileNameW(nullptr, buf, sizeof(buf) / sizeof(buf[0]));

                        if (pathLength > 0) {
                            modPath.set(buf, buf + pathLength);
                            modPath.replaceSeprators();
                        }
                    }

                    if(modPath.isNotEmpty()) {
                        filename.set(modPath.fileName());
                        filename.removeExtension();
                    }
                    
                    if(filename.isEmpty()) {
                        filename.set(L"unknown");
                    }

                    filename.appendFmt(L"_%S_%S.dmp", date.toString(date_txt), time.toString(time_txt));

                    X_LOG_BULLET;
                    X_LOG0("ExceptionHandler", "Writing crash dump to file: \"%ls\"", filename.c_str());

                    if (!debugging::WriteMiniDump(filename, debugging::DumpType::Full, exceptionPointers)) {
                        X_ERROR("ExceptionHandler", "Could not write crash dump.");
                    }

                    X_LOG0("ExceptionHandler", "Quitting the application.");
                }

                core::msgbox::show("Unhandled exception! The program will now close."_sv,
                    X_ENGINE_NAME " Fatal Error"_sv,
                    core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
                    core::msgbox::Buttons::OK);
            }

            return 1;
        }

    } // namespace internal

#endif

    LONG WINAPI HandleException(_EXCEPTION_POINTERS* exceptionPointers)
    {
#if X_ENABLE_UNHANDLED_EXCEPTION_HANDLER
        return internal::HandleException(exceptionPointers);
#else
        X_UNUSED(exceptionPointers);
        return 1;
#endif
    }

    void Startup(void)
    {
#if X_ENABLE_UNHANDLED_EXCEPTION_HANDLER
        //	X_LOG0( "ExceptionHandler", "Registering exception handler." );

        g_oldExceptionFilter = SetUnhandledExceptionFilter(HandleException);
#endif
    }

    void Shutdown(void)
    {
#if X_ENABLE_UNHANDLED_EXCEPTION_HANDLER
        //	X_LOG0( "ExceptionHandler", "Unregistering exception handler." );

        SetUnhandledExceptionFilter(g_oldExceptionFilter);
#endif
    }

} // namespace exceptionHandler

X_NAMESPACE_END