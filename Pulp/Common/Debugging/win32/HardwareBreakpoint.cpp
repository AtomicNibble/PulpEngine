#include "EngineCommon.h"

#include "HardwareBreakpoint.h"

#include "Util\LastError.h"
#include "Util\BitUtil.h"

#include "ICore.h"

X_NAMESPACE_BEGIN(core)

namespace hardwareBP
{
    namespace
    {
        template<typename T>
        T FindFreeReg(T dr7)
        {
            for (int32_t i = 0; i < 4; i++) {
                if (!bitUtil::IsBitSet<T>(dr7, 2 * i)) {
                    return i;
                }
            }
            return 4;
        }

        template<typename T>
        T FindUsedReg(T add, CONTEXT& ct)
        {
            if (ct.Dr0 == add)
                return 0;
            else if (ct.Dr1 == add)
                return 1;
            else if (ct.Dr2 == add)
                return 2;
            else if (ct.Dr3 == add)
                return 3;
            return 4;
        }

        namespace internal
        {
            template<size_t N>
            struct Implementation
            {
            };

            /// Template specialization for 64bit
            template<>
            struct Implementation<8u>
            {
#ifdef _WIN64
                static void Install(void* address, Condition::Enum cond, Size::Enum size)
                {
                    CONTEXT ct = {0};
                    ct.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                    HANDLE hThread = GetCurrentThread();

                    if (GetThreadContext(hThread, &ct)) {
                        DWORD64 reg = FindFreeReg(ct.Dr7);

                        if (reg == 4) {
                            X_ERROR("HardwareBreakpoint", "Failed to install breakpoint, All 4 registers are in use");
                        }
                        else {
                            switch (reg) {
                                case 0:
                                    ct.Dr0 = reinterpret_cast<DWORD64>(address);
                                    break;
                                case 1:
                                    ct.Dr1 = reinterpret_cast<DWORD64>(address);
                                    break;
                                case 2:
                                    ct.Dr2 = reinterpret_cast<DWORD64>(address);
                                    break;
                                case 3:
                                    ct.Dr3 = reinterpret_cast<DWORD64>(address);
                                    break;

                                default:
                                    X_NO_SWITCH_DEFAULT;
                            }

                            ct.Dr7 = bitUtil::SetBit<DWORD64>(ct.Dr7, safe_static_cast<uint, DWORD64>(2 * reg));
                            ct.Dr7 = bitUtil::ReplaceBits<DWORD64>(ct.Dr7, safe_static_cast<uint, DWORD64>((4 * reg) + 16), 2u, safe_static_cast<DWORD, int>(cond));
                            ct.Dr7 = bitUtil::ReplaceBits<DWORD64>(ct.Dr7, safe_static_cast<uint, DWORD64>((4 * reg) + 18), 2u, safe_static_cast<DWORD, int>(size));

                            if (!SetThreadContext(hThread, &ct)) {
                                lastError::Description Dsc;
                                X_ERROR("HardwareBreakpoint", "Failed to set thread conext. Error: %s", lastError::ToString(Dsc));
                            }
                        }
                    }
                    else {
                        lastError::Description Dsc;
                        X_ERROR("HardwareBreakpoint", "Failed to retrive thread conext. Error: %s", lastError::ToString(Dsc));
                    }
                }

                static void Uninstall(void* address)
                {
                    CONTEXT ct = {0};
                    ct.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                    HANDLE hThread = GetCurrentThread();

                    if (GetThreadContext(hThread, &ct)) {
                        DWORD64 reg = FindUsedReg(reinterpret_cast<DWORD64>(address), ct);

                        if (reg == 4) {
                            X_ERROR("HardwareBreakpoint", "Cannot remove hardware breakpoint, No breakpoint installed at 0x%08X ", address);
                            return;
                        }

                        ct.Dr7 = bitUtil::ClearBit<DWORD64>(ct.Dr7, safe_static_cast<uint, DWORD64>((2 * reg)));

                        if (!SetThreadContext(hThread, &ct)) {
                            lastError::Description Dsc;
                            X_ERROR("HardwareBreakpoint", "Failed to set thread conext. Error: %s", lastError::ToString(Dsc));
                        }
                    }
                    else {
                        lastError::Description Dsc;
                        X_ERROR("HardwareBreakpoint", "Failed to retrive thread conext. Error: %s", lastError::ToString(Dsc));
                    }
                }
#endif // !_WIN64
            };

            /// Template specialization for 32bit
            template<>
            struct Implementation<4u>
            {
#ifndef _WIN64

                static void Install(void* address, Condition::Enum cond, Size::Enum size)
                {
                    CONTEXT ct = {0};
                    ct.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                    HANDLE hThread = GetCurrentThread();

                    if (GetThreadContext(hThread, &ct)) {
                        DWORD reg = FindFreeReg(ct.Dr7);

                        if (reg == 4) {
                            X_ERROR("HardwareBreakpoint", "Failed to install breakpoint, All 4 registers are in use");
                        }
                        else {
                            switch (reg) {
                                case 0:
                                    ct.Dr0 = reinterpret_cast<DWORD>(address);
                                    break;
                                case 1:
                                    ct.Dr1 = reinterpret_cast<DWORD>(address);
                                    break;
                                case 2:
                                    ct.Dr2 = reinterpret_cast<DWORD>(address);
                                    break;
                                case 3:
                                    ct.Dr3 = reinterpret_cast<DWORD>(address);
                                    break;

                                default:
                                    X_NO_SWITCH_DEFAULT;
                            }

                            ct.Dr7 = bitUtil::SetBit<DWORD>(ct.Dr7, 2 * reg);
                            ct.Dr7 = bitUtil::ReplaceBits<DWORD>(ct.Dr7, 4 * reg + 16, 2u, safe_static_cast<DWORD, int>(cond));
                            ct.Dr7 = bitUtil::ReplaceBits<DWORD>(ct.Dr7, 4 * reg + 18, 2u, safe_static_cast<DWORD, int>(size));

                            if (!SetThreadContext(hThread, &ct)) {
                                lastError::Description Dsc;
                                X_ERROR("HardwareBreakpoint", "Failed to set thread conext. Error: %s", lastError::ToString(Dsc));
                            }
                        }
                    }
                    else {
                        lastError::Description Dsc;
                        X_ERROR("HardwareBreakpoint", "Failed to retrive thread conext. Error: %s", lastError::ToString(Dsc));
                    }
                }

                static void Uninstall(void* address)
                {
                    CONTEXT ct = {0};
                    ct.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                    HANDLE hThread = GetCurrentThread();

                    if (GetThreadContext(hThread, &ct)) {
                        DWORD reg = FindUsedReg((DWORD)address, ct);

                        if (reg == 4) {
                            X_ERROR("HardwareBreakpoint", "Cannot remove hardware breakpoint, No breakpoint installed at 0x%08X ", address);
                        }
                        else {
                            ct.Dr7 = bitUtil::ClearBit<DWORD>(ct.Dr7, 2 * reg);

                            if (!SetThreadContext(hThread, &ct)) {
                                lastError::Description Dsc;
                                X_ERROR("HardwareBreakpoint", "Failed to set thread conext. Error: %s", lastError::ToString(Dsc));
                            }
                        }
                    }
                    else {
                        lastError::Description Dsc;
                        X_ERROR("HardwareBreakpoint", "Failed to retrive thread conext. Error: %s", lastError::ToString(Dsc));
                    }
                }
#endif // !_WIN64
            };

        } // namespace internal
    }     // namespace

    void Install(void* address, Condition::Enum cond, Size::Enum size)
    {
        internal::Implementation<sizeof(address)>::Install(address, cond, size);
    }

    void Uninstall(void* address)
    {
        internal::Implementation<sizeof(address)>::Uninstall(address);
    }

} // namespace hardwareBP

X_NAMESPACE_END