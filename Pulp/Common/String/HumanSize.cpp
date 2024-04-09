#include "EngineCommon.h"
#include "HumanSize.h"

X_NAMESPACE_BEGIN(core)

namespace HumanSize
{
    const char* toString(Str& str, uint32_t numBytes)
    {
        str.clear();

        // Kibi not Kilo

        // i use each type until there is 10,240 of them.
        if (numBytes <= 10240) {
            str.setFmt("%" PRIu32 "bytes", numBytes);
        }
        else if (numBytes <= 10485760) {
            str.setFmt("%.2fKB", static_cast<double>(numBytes) / 1024);
        }
        else {
            str.setFmt("%.2fMB", (static_cast<double>(numBytes) / 1024) / 1024);
        }

        return str.c_str();
    }

    const char* toString(Str& str, int32_t numBytes)
    {
        str.clear();

        // Kibi not Kilo

        // i use each type until there is 10,240 of them.
        if (numBytes <= 10240) {
            str.setFmt("%" PRIu32 "bytes", numBytes);
        }
        else if (numBytes <= 10485760) {
            str.setFmt("%.2fKB", static_cast<double>(numBytes) / 1024);
        }
        else {
            str.setFmt("%.2fMB", (static_cast<double>(numBytes) / 1024) / 1024);
        }

        return str.c_str();
    }


    const char* toString(Str& str, uint64_t numBytes)
    {
        str.clear();

        // Kibi not Kilo

        // i use each type until there is 10,240 of them.
        if (numBytes <= 10240) {
            str.setFmt("%" PRIu64 "bytes", numBytes);
        }
        else if (numBytes <= 10485760) {
            str.setFmt("%.2fKB", static_cast<double>(numBytes) / 1024);
        }
        else if (numBytes <= 10737418240) {
            str.setFmt("%.2fMB", (static_cast<double>(numBytes) / 1024) / 1024);
        }
        else if (numBytes <= 10995116277760) {
            str.setFmt("%.2fGB", ((static_cast<double>(numBytes) / 1024) / 1024) / 1024);
        }
        else if (numBytes <= 11258999068426240) {
            str.setFmt("%.2fTB", (((static_cast<double>(numBytes) / 1024) / 1024) / 1024) / 1024);
        }
        else {
            str.setFmt("%.2fGB", (((static_cast<double>(numBytes) / 1024) / 1024) / 1024) / 1024);
        }

        return str.c_str();
    }

    const char* toString(Str& str, int64_t numBytes)
    {
        str.clear();

        // Kibi not Kilo
        const char* pSign = "";

        if (core::bitUtil::isSignBitSet(numBytes)) {
            pSign = "-";
            numBytes = -numBytes;
        }

        // i use each type until there is 10,240 of them.
        if (numBytes <= 10240) {
            str.setFmt("%s%" PRIi64 "bytes", pSign, numBytes);
        }
        else if (numBytes <= 10485760) {
            str.setFmt("%s%.2fKB", pSign, static_cast<double>(numBytes) / 1024);
        }
        else if (numBytes <= 10737418240) {
            str.setFmt("%s%.2fMB", pSign, (static_cast<double>(numBytes) / 1024) / 1024);
        }
        else if (numBytes <= 10995116277760) {
            str.setFmt("%s%.2fGB", pSign, ((static_cast<double>(numBytes) / 1024) / 1024) / 1024);
        }
        else if (numBytes <= 11258999068426240) {
            str.setFmt("%s%.2fTB", pSign, (((static_cast<double>(numBytes) / 1024) / 1024) / 1024) / 1024);
        }
        else {
            str.setFmt("%s%.2fGB", pSign, (((static_cast<double>(numBytes) / 1024) / 1024) / 1024) / 1024);
        }

        return str.c_str();
    }

} // namespace HumanSize

X_NAMESPACE_END;
