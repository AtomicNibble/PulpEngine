#pragma once

X_NAMESPACE_BEGIN(core)


class TelemSpikeDetector
{
public:
    TelemSpikeDetector(TraceContexHandle ctx, const char* pMsg, float thresholdMS = 5) :
        ctx_(ctx),
        begin_(ttFastTime()),
        pMsg_(pMsg),
        msThreshold_(thresholdMS)
    {
    }

    ~TelemSpikeDetector()
    {
        float time = ttFastTimeToMs(ctx_, ttFastTime() - begin_);
        if (time >= msThreshold_)
        {
            ttWarning(ctx_, "(spike)%s %.2fms", pMsg_, time);
        }
    }

private:
    TraceContexHandle ctx_;
    tt_uint64 begin_;
    const char* pMsg_;
    float msThreshold_;
};


X_NAMESPACE_END
