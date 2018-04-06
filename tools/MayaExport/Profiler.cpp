#include "stdafx.h"
#include "Profiler.h"

#include "MayaUtil.h"

MayaProfiler::MayaProfiler(const char* name) :
    name_(name)
{
}

MayaProfiler::~MayaProfiler()
{
    float ms = timer_.GetMilliSeconds();

    core::StackString512 msg;

    if (name_ != nullptr) {
        msg.append(name_);
    }

    msg.appendFmt(" (%gms)", ms);

    MayaUtil::MayaPrintMsg(msg.c_str());
}