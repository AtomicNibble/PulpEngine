#include "stdafx.h"
#include "TelemetryViewer.h"

X_NAMESPACE_BEGIN(telemetry)


namespace
{
    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    static const char* IntTable100 =
        "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

    X_DISABLE_WARNING(4244)

    using StringBuf = core::StackString<64,char>;


    inline void PrintTinyInt(StringBuf& buf, uint64_t v)
    {
        if (v >= 10)
        {
            buf.append('0' + v / 10, 1);
        }
        buf.append('0' + v % 10, 1);
    }

    inline void PrintTinyInt0(StringBuf& buf, uint64_t v)
    {
        if (v >= 10)
        {
            buf.append('0' + v / 10, 1);
        }
        else
        {
            buf.append('0', 1);
        }
        buf.append('0' + v % 10, 1);
    }

    inline void PrintSmallInt(StringBuf& buf, uint64_t v)
    {
        if (v >= 100)
        {
            buf.append(IntTable100 + v / 10 * 2, 2);
        }
        else if (v >= 10)
        {
            buf.append('0' + v / 10, 1);
        }
        buf.append('0' + v % 10, 1);
    }

    inline void PrintFrac00(StringBuf& buf, uint64_t v)
    {
        buf.append('.', 1);
        v += 5;
        if (v / 10 % 10 == 0)
        {
            buf.append('0' + v / 100, 1);
        }
        else
        {
            buf.append(IntTable100 + v / 10 * 2, 2);
        }
    }

    inline void PrintFrac0(StringBuf& buf, uint64_t v)
    {
        buf.append('.', 1);
        buf.append('0' + (v + 50) / 100, 1);
    }

    inline void PrintSmallIntFrac(StringBuf& buf, uint64_t v)
    {
        uint64_t in = v / 1000;
        uint64_t fr = v % 1000;
        if (fr >= 995)
        {
            PrintSmallInt(buf, in + 1);
        }
        else
        {
            PrintSmallInt(buf, in);
            if (fr > 5)
            {
                PrintFrac00(buf, fr);
            }
        }
    }

    inline void PrintSecondsFrac(StringBuf& buf, uint64_t v)
    {
        uint64_t in = v / 1000;
        uint64_t fr = v % 1000;
        if (fr >= 950)
        {
            PrintTinyInt0(buf, in + 1);
        }
        else
        {
            PrintTinyInt0(buf, in);
            if (fr > 50)
            {
                PrintFrac0(buf, fr);
            }
        }
    }

    const char* TimeToString(StringBuf& buf, int64_t _ns)
    {
        buf.clear();

        uint64_t ns;
        if (_ns < 0)
        {
            buf.append('-', 1);
            ns = -_ns;
        }
        else
        {
            ns = _ns;
        }

        if (ns < 1000)
        {
            PrintSmallInt(buf, ns);
            buf.append(" ns");
        }
        else if (ns < 1000ll * 1000)
        {
            PrintSmallIntFrac(buf, ns);
#ifdef TRACY_EXTENDED_FONT
            buf.append(" \xce\xbcs");
#else
            buf.append(" us");
#endif
        }
        else if (ns < 1000ll * 1000 * 1000)
        {
            PrintSmallIntFrac(buf, ns / 1000);
            buf.append(" ms");
        }
        else if (ns < 1000ll * 1000 * 1000 * 60)
        {
            PrintSmallIntFrac(buf, ns / (1000ll * 1000));
            buf.append(" s");
        }
        else if (ns < 1000ll * 1000 * 1000 * 60 * 60)
        {
            const auto m = int64_t(ns / (1000ll * 1000 * 1000 * 60));
            const auto s = int64_t(ns - m * (1000ll * 1000 * 1000 * 60)) / (1000ll * 1000);
            PrintTinyInt(buf, m);
            buf.append(':', 1);
            PrintSecondsFrac(buf, s);
        }
        else if (ns < 1000ll * 1000 * 1000 * 60 * 60 * 24)
        {
            const auto h = int64_t(ns / (1000ll * 1000 * 1000 * 60 * 60));
            const auto m = int64_t(ns / (1000ll * 1000 * 1000 * 60) - h * 60);
            const auto s = int64_t(ns / (1000ll * 1000 * 1000) - h * (60 * 60) - m * 60);
            PrintTinyInt(buf, h);
            buf.append(':', 1);
            PrintTinyInt0(buf, m);
            buf.append(':', 1);
            PrintTinyInt0(buf, s);
        }
        else
        {
            const auto d = int64_t(ns / (1000ll * 1000 * 1000 * 60 * 60 * 24));
            const auto h = int64_t(ns / (1000ll * 1000 * 1000 * 60 * 60) - d * 24);
            const auto m = int64_t(ns / (1000ll * 1000 * 1000 * 60) - d * (60 * 24) - h * 60);
            const auto s = int64_t(ns / (1000ll * 1000 * 1000) - d * (60 * 60 * 24) - h * (60 * 60) - m * 60);
            if (d < 1000)
            {
                PrintSmallInt(buf, d);
                buf.append('d', 1);
            }
            else
            {
                buf.appendFmt("%" PRIi64 "d", d);
            }

            PrintTinyInt0(buf, h);
            buf.append(':', 1);
            PrintTinyInt0(buf, m);
            buf.append(':', 1);
            PrintTinyInt0(buf, s);
        }

        return buf.c_str();
    }

    
    const char* IntToString(StringBuf& buf, int32_t val)
    {
        buf.setFmt("%" PRIi32, val);
        return buf.c_str();
    }

    const char* RealToString(StringBuf& buf, double val, bool separator)
    {
        X_UNUSED(separator);

        buf.setFmt("%f", val);
        
#if false
        auto ptr = buf;
        if (*ptr == '-') ptr++;

        const auto vbegin = ptr;

        if (separator)
        {
            while (*ptr != '\0' && *ptr != ',' && *ptr != '.') ptr++;
            auto end = ptr;
            while (*end != '\0') end++;
            auto sz = end - ptr;

            while (ptr - vbegin > 3)
            {
                ptr -= 3;
                memmove(ptr + 1, ptr, sz);
                *ptr = ',';
                sz += 4;
            }
        }

        while (*ptr != '\0' && *ptr != ',' && *ptr != '.') ptr++;

        if (*ptr == '\0') return buf;
        while (*ptr != '\0') ptr++;
        ptr--;
        while (*ptr == '0') ptr--;
        if (*ptr != '.' && *ptr != ',') ptr++;
        *ptr = '\0';
#endif

        return buf.c_str();
    }

    void TextDisabledUnformatted(const char* begin, const char* end = nullptr)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GImGui->Style.Colors[ImGuiCol_TextDisabled]);
        ImGui::TextUnformatted(begin, end);
        ImGui::PopStyleColor();
    }

    void TextFocused(const char* label, const char* value)
    {
        TextDisabledUnformatted(label);
        ImGui::SameLine();
        ImGui::TextUnformatted(value);
    }

    void TextFocusedFmt(const char* label, const char* pFmt, ...)
    {
        TextDisabledUnformatted(label);
        ImGui::SameLine();

        core::StackString256 str;
        va_list args;
        va_start(args, pFmt);
        str.setFmt(pFmt, args);
        va_end(args);

        ImGui::TextUnformatted(str.c_str(), str.end());
    }

    void DrawHelpMarker(const char* desc)
    {
        TextDisabledUnformatted("(?)");
        if (ImGui::IsItemHovered())
        {
            const auto ty = ImGui::GetFontSize();
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(450.0f * ty / 15.f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void DrawTextContrast(ImDrawList* draw, const ImVec2& pos, uint32_t color, const char* text)
    {
        draw->AddText(pos + ImVec2(1, 1), 0x88000000, text);
        draw->AddText(pos, color, text);
    }


    bool Splitter(bool split_vertically, float thickness, float* size1, float* size2,
        float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    // ---------------------------


    int32_t GetFrameWidth(int32_t frameScale)
    {
        return frameScale == 0 ? 4 : (frameScale < 0 ? 6 : 1);
    }

    int32_t GetFrameGroup(int32_t frameScale)
    {
        return frameScale < 2 ? 1 : (1 << (frameScale - 1));
    }

    ImU32 GetFrameColor(uint64_t frameTime)
    {
        enum { BestTime = 1000 * 1000 * 1000 / 143 };
        enum { GoodTime = 1000 * 1000 * 1000 / 59 };
        enum { BadTime = 1000 * 1000 * 1000 / 29 };

        return frameTime > BadTime ? 0xFF2222DD :
            frameTime > GoodTime ? 0xFF22DDDD :
            frameTime > BestTime ? 0xFF22DD22 : 0xFFDD9900;
    }

    int64_t GetFrameTime(size_t idx)
    {
        X_UNUSED(idx);
        return 1000 * 1000 * (10 + (idx % 10));
    }

    int64_t GetFrameBegin(size_t idx)
    {
        X_UNUSED(idx);
        return 1000 * 1000 * (10 + (idx % 10));
    }

    int64_t GetFrameEnd(size_t idx)
    {
        X_UNUSED(idx);
        return GetFrameBegin(idx) + 1024;
    }

    int64_t GetTimeBegin(void)
    {
        return GetFrameBegin(0);
    }

    uint64_t GetFrameOffset()
    {
        return 0;
    }

    std::pair<int, int> GetFrameRange(int64_t from, int64_t to)
    {
        X_UNUSED(from, to);
        return { 10, 40 };
    }

    const char* GetThreadString(uint64_t id)
    {
        X_UNUSED(id);
        return "meow";
    }

    namespace HumanNumber
    {
        using Str = core::StackString<96, char>;

        const char* toString(Str& str, int64_t num)
        {
            str.clear();

            int64_t n2 = 0;
            int64_t scale = 1;
            if (num < 0) {
                str.append('-', 1);
                num = -num;
            }
            while (num >= 1000) {
                n2 = n2 + scale * (num % 1000);
                num /= 1000;
                scale *= 1000;
            }

            str.appendFmt("%" PRIi64, num);
            while (scale != 1) {
                scale /= 1000;
                num = n2 / scale;
                n2 = n2 % scale;
                str.appendFmt(",%03" PRIi64, num);
            }

            return str.c_str();
        }

    }

    void DrawZigZag(ImDrawList* draw, const ImVec2& wpos, double start, double end, double h, uint32_t color)
    {
        int mode = 0;
        while (start < end)
        {
            double step = std::min(end - start, mode == 0 ? h / 2 : h);
            switch (mode)
            {
                case 0:
                    draw->AddLine(wpos + ImVec2(start, 0), wpos + ImVec2(start + step, round(-step)), color);
                    mode = 1;
                    break;
                case 1:
                    draw->AddLine(wpos + ImVec2(start, round(-h / 2)), wpos + ImVec2(start + step, round(step - h / 2)), color);
                    mode = 2;
                    break;
                case 2:
                    draw->AddLine(wpos + ImVec2(start, round(h / 2)), wpos + ImVec2(start + step, round(h / 2 - step)), color);
                    mode = 1;
                    break;
                default:
                    X_ASSERT_UNREACHABLE();
                    break;
            };
            start += step;
        }
    }

    uint32_t GetColorMuted(uint32_t color, bool active)
    {
        if (active) {
            return 0xFF000000 | color;
        }
    
        return 0x66000000 | color;
    }

    using FrameTextStrBuf = core::StackString<64, char>;

    const char* GetFrameText(FrameTextStrBuf& buf, int64_t durationNS, int64_t frameNum)
    {
        StringBuf strBuf;
        buf.setFmt("Frame %" PRIi64 " (%s)", frameNum, TimeToString(strBuf, durationNS));
        return buf.c_str();
    }

    enum { MinVisSize = 3 };
    enum { MinFrameSize = 5 };



} // namespace

void ZoomToRange(TraceView& view, int64_t start, int64_t end)
{
    if (start == end)
    {
        end = start + 1;
    }

    view.paused_ = true;
    view.highlightZoom_.active = false;
    view.zoomAnim_.active = true;
    view.zoomAnim_.start0 = view.zvStartNS_;
    view.zoomAnim_.start1 = start;
    view.zoomAnim_.end0 = view.zvEndNS_;
    view.zoomAnim_.end1 = end;
    view.zoomAnim_.progress = 0;

    const auto d0 = double(view.zoomAnim_.end0 - view.zoomAnim_.start0);
    const auto d1 = double(view.zoomAnim_.end1 - view.zoomAnim_.start1);
    const auto diff = d0 > d1 ? d0 / d1 : d1 / d0;
    view.zoomAnim_.lenMod = 5.0 / log10(diff);
}

void ZoomToZone(TraceView& view, const ZoneData& zone)
{
    if (zone.endNano - zone.startNano <= 0) {
        return;
    }

    ZoomToRange(view, zone.startNano, zone.endNano);
}


void HandleZoneViewMouse(TraceView& view, int64_t timespan, const ImVec2& wpos, float w, double& pxns)
{
    auto& io = ImGui::GetIO();

    const auto nspx = double(timespan) / w;

    if (ImGui::IsMouseClicked(0))
    {
        view.highlight_.active = true;
        view.highlight_.start = view.highlight_.end = view.zvStartNS_ + (io.MousePos.x - wpos.x) * nspx;
    }
    else if (ImGui::IsMouseDragging(0, 0))
    {
        view.highlight_.end = view.zvStartNS_ + (io.MousePos.x - wpos.x) * nspx;
    }
    else
    {
        view.highlight_.active = false;
    }

    if (ImGui::IsMouseClicked(2))
    {
        view.highlightZoom_.active = true;
        view.highlightZoom_.start = view.highlightZoom_.end = view.zvStartNS_ + (io.MousePos.x - wpos.x) * nspx;
    }
    else if (ImGui::IsMouseDragging(2, 0))
    {
        view.highlightZoom_.end = view.zvStartNS_ + (io.MousePos.x - wpos.x) * nspx;
    }
    else if (view.highlightZoom_.active)
    {
        if (view.highlightZoom_.start != view.highlightZoom_.end)
        {
            const auto s = std::min(view.highlightZoom_.start, view.highlightZoom_.end);
            const auto e = std::max(view.highlightZoom_.start, view.highlightZoom_.end);

            // ZoomToRange disables view.highlightZoom_.active
            if (io.KeyCtrl)
            {
                const auto tsOld = view.zvEndNS_ - view.zvStartNS_;
                const auto tsNew = e - s;
                const auto mul = double(tsOld) / tsNew;
                const auto left = s - view.zvStartNS_;
                const auto right = view.zvEndNS_ - e;

                ZoomToRange(view, view.zvStartNS_ - left * mul, view.zvEndNS_ + right * mul);
            }
            else
            {
                ZoomToRange(view, s, e);
            }
        }
        else
        {
            view.highlightZoom_.active = false;
        }
    }

    if (ImGui::IsMouseDragging(1, 0))
    {
        view.zoomAnim_.active = false;
        view.paused_ = true;
        const auto delta = ImGui::GetMouseDragDelta(1, 0);
        const auto dpx = int64_t(delta.x * nspx);
        if (dpx != 0)
        {
            view.zvStartNS_ -= dpx;
            view.zvEndNS_ -= dpx;
            io.MouseClickedPos[1].x = io.MousePos.x;
        }
        if (delta.y != 0)
        {
            auto y = ImGui::GetScrollY();
            ImGui::SetScrollY(y - delta.y);
            io.MouseClickedPos[1].y = io.MousePos.y;
        }
    }

    const auto wheel = io.MouseWheel;
    if (wheel != 0)
    {
        view.zoomAnim_.active = false;
        view.paused_ = true;
        const double mouse = io.MousePos.x - wpos.x;
        const auto p = mouse / w;
        const auto p1 = timespan * p;
        const auto p2 = timespan - p1;
        if (wheel > 0)
        {
            view.zvStartNS_ += int64_t(p1 * 0.25);
            view.zvEndNS_ -= int64_t(p2 * 0.25);
        }
        else if (timespan < 1000ll * 1000 * 1000 * 60 * 60)
        {
            view.zvStartNS_ -= std::max(int64_t(1), int64_t(p1 * 0.25));
            view.zvEndNS_ += std::max(int64_t(1), int64_t(p2 * 0.25));
        }
        timespan = view.zvEndNS_ - view.zvStartNS_;
        pxns = w / double(timespan);
    }
}

void ZoneTooltip(TraceView& view, const ZoneData& zone)
{
    X_UNUSED(view);

    const int64_t cycles = zone.endTicks - zone.startTicks;

    const int64_t end = zone.endNano;
    const int64_t time = end - zone.startNano;
    const int64_t childTime = 0;
    const int64_t selftime = time - childTime;

    StringBuf strBuf;

    ImGui::BeginTooltip();
  
        ImGui::TextUnformatted("Function name");
        ImGui::Separator();
        ImGui::Text("%s:%i", "FlyingGoat\\stu.cpp", zone.lineNo);
        TextFocused("Thread:", "Goat Thread");
        ImGui::SameLine();
        ImGui::TextDisabled("(0x%" PRIX32 ")", 0x12345);
        ImGui::Separator();
        TextFocused("Execution time:", TimeToString(strBuf, time));
        ImGui::SameLine();
        TextFocusedFmt("Cycles:", "%" PRId64, cycles);
        TextFocused("Self time:", TimeToString(strBuf, selftime));
        ImGui::SameLine();
        ImGui::TextDisabled("(%.2f%%)", 100.f * selftime / time);
    
    ImGui::EndTooltip();
}


// Draws like a overview of all the frames, so easy to find peaks.
void DrawFrames(TraceView& view)
{
    X_UNUSED(view);

}

bool DrawZoneFramesHeader(TraceView& view)
{
    const auto wpos = ImGui::GetCursorScreenPos();
    const auto w = ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ScrollbarSize;
    auto* draw = ImGui::GetWindowDrawList();
    const auto ty = ImGui::GetFontSize();

    ImGui::InvisibleButton("##zoneFrames", ImVec2(w, ty * 1.5));
    bool hover = ImGui::IsItemHovered();

    auto timespan = view.GetVisiableNS();
    auto pxns = w / double(timespan);


    if (hover) {
       HandleZoneViewMouse(view, timespan, wpos, w, pxns);
    }

    {
        const auto nspx = 1.0 / pxns;
        const auto scale = std::max(0.0, math<double>::round(log10(nspx) + 2));
        const auto step = pow(10, scale);

        const auto dx = step * pxns;
        double x = 0;
        int32_t tw = 0;
        int32_t tx = 0;
        int64_t tt = 0;

        StringBuf strBuf;

        while (x < w)
        {
            draw->AddLine(wpos + ImVec2(x, 0), wpos + ImVec2(x, math<double>::round(ty * 0.5)), 0x66FFFFFF);
            if (tw == 0)
            {
                const auto t = view.GetVisibleStartNS();
                TimeToString(strBuf, t);

                auto col = 0x66FFFFFF;

                // TODO: add some animation to this text so it gets brighter when value is changing.

                if (t >= 0) // prefix the shit.
                {
                    StringBuf strBuf1;
                    strBuf1.setFmt("+%s", strBuf.c_str());
                    strBuf = strBuf1;
                    col = 0xaaFFFFFF;
                }
                else
                {
                    col = 0xaa2020FF;
                }

                draw->AddText(wpos + ImVec2(x, math<double>::round(ty * 0.5)), col, strBuf.begin(), strBuf.end());
                tw = ImGui::CalcTextSize(strBuf.begin(), strBuf.end()).x;
            }
            else if (x > tx + tw + ty * 2)
            {
                tx = x;
                TimeToString(strBuf, tt);
                draw->AddText(wpos + ImVec2(x, math<double>::round(ty * 0.5)), 0x66FFFFFF, strBuf.begin(), strBuf.end());
                tw = ImGui::CalcTextSize(strBuf.begin(), strBuf.end()).x;
            }

            if (scale != 0)
            {
                for (int32_t i = 1; i < 5; i++)
                {
                    draw->AddLine(wpos + ImVec2(x + i * dx / 10, 0), wpos + ImVec2(x + i * dx / 10, round(ty * 0.25)), 0x33FFFFFF);
                }

                draw->AddLine(wpos + ImVec2(x + 5 * dx / 10, 0), wpos + ImVec2(x + 5 * dx / 10, round(ty * 0.375)), 0x33FFFFFF);

                for (int32_t i = 6; i < 10; i++)
                {
                    draw->AddLine(wpos + ImVec2(x + i * dx / 10, 0), wpos + ImVec2(x + i * dx / 10, round(ty * 0.25)), 0x33FFFFFF);
                }
            }

            x += dx;
            tt += step;
        }
    }

    return hover;
}

bool DrawZoneFrames(TraceView& view)
{
    const auto wpos = ImGui::GetCursorScreenPos();
    const auto w = ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ScrollbarSize;
    const auto wh = ImGui::GetContentRegionAvail().y;
    const auto ty = ImGui::GetFontSize();
    auto draw = ImGui::GetWindowDrawList();

    ImGui::InvisibleButton("##zoneFrames", ImVec2(w, ty));
    bool hover = ImGui::IsItemHovered();

    auto timespan = view.GetVisiableNS();
    auto pxns = w / double(timespan);

    if (hover)
    {
        HandleZoneViewMouse(view, timespan, wpos, w, pxns);
    }

    const auto nspx = 1.0 / pxns;

    int64_t prev = -1;
    int64_t prevEnd = -1;
    int64_t endPos = -1;

    StringBuf strBuf;
    FrameTextStrBuf frameStrBuf;

    bool tooltipDisplayed = false;
    const bool activeFrameSet = false;
    const bool continuous = true; // TODO: ?

    const auto inactiveColor = GetColorMuted(0x888888, activeFrameSet);
    const auto activeColor = GetColorMuted(0xFFFFFF, activeFrameSet);
    const auto redColor = GetColorMuted(0x4444FF, activeFrameSet);

    X_DISABLE_WARNING(4127) // conditional expression is constant

    // draw the ticks / frames.
    if (view.segments.isNotEmpty())
    {
        auto& segment = view.segments.front();

        // the ticks should be sorted.
        auto& ticks = segment.ticks;

        auto it = std::lower_bound(ticks.begin(), ticks.end(), view.GetVisibleStartNS(), [](const TickData& tick, int64_t val) {
            return tick.endNano < val;
        });

        if (it != ticks.end())
        {
            // draw them?
            // 1000000
            // 246125553
            while (it != ticks.end() && it->startNano < view.zvEndNS_)
            {
                auto& tick = *it;
                ++it;

                const auto ftime = tick.endNano - tick.startNano;
                const auto fbegin = tick.startNano;
                const auto fend = tick.endNano;
                const auto fsz = pxns * ftime;

                auto offset = std::distance(ticks.begin(), it);

                {
                    if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2((fbegin - view.zvStartNS_) * pxns, 0), wpos + ImVec2((fend - view.zvStartNS_) * pxns, ty)))
                    {
                        tooltipDisplayed = true;


                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(GetFrameText(frameStrBuf, ftime, offset));
                        ImGui::Separator();
                        TextFocused("Duration:", TimeToString(strBuf, ftime));
                        TextFocused("Offset:", TimeToString(strBuf, fbegin));
                        ImGui::EndTooltip();

                        if (ImGui::IsMouseClicked(2))
                        {
                            ZoomToRange(view, fbegin, fend);
                        }
                    }

                    if (fsz < MinFrameSize)
                    {
                        if (!continuous && prev != -1)
                        {
                            if ((fbegin - prevEnd) * pxns >= MinFrameSize)
                            {
                                DrawZigZag(draw, wpos + ImVec2(0, round(ty / 2)), (prev - view.zvStartNS_) * pxns, (prevEnd - view.zvStartNS_) * pxns, ty / 4, inactiveColor);
                                prev = -1;
                            }
                            else
                            {
                                prevEnd = std::max<int64_t>(fend, fbegin + MinFrameSize * nspx);
                            }
                        }

                        if (prev == -1)
                        {
                            prev = fbegin;
                            prevEnd = std::max<int64_t>(fend, fbegin + MinFrameSize * nspx);
                        }

                        continue;
                    }

                    if (prev != -1)
                    {
                        if (continuous)
                        {
                            DrawZigZag(draw, wpos + ImVec2(0, round(ty / 2)), (prev - view.zvStartNS_) * pxns, (fbegin - view.zvStartNS_) * pxns, ty / 4, inactiveColor);
                        }
                        else
                        {
                            DrawZigZag(draw, wpos + ImVec2(0, round(ty / 2)), (prev - view.zvStartNS_) * pxns, (prevEnd - view.zvStartNS_) * pxns, ty / 4, inactiveColor);
                        }
                        prev = -1;
                    }

                    if (activeFrameSet)
                    {
                        if (fbegin >= view.zvStartNS_ && endPos != fbegin)
                        {
                            draw->AddLine(wpos + ImVec2((fbegin - view.zvStartNS_) * pxns, 0), wpos + ImVec2((fbegin - view.zvStartNS_) * pxns, wh), 0x22FFFFFF);
                        }
                        if (fend <= view.zvEndNS_)
                        {
                            draw->AddLine(wpos + ImVec2((fend - view.zvStartNS_) * pxns, 0), wpos + ImVec2((fend - view.zvStartNS_) * pxns, wh), 0x22FFFFFF);
                        }
                        endPos = fend;
                    }

                    auto buf = GetFrameText(frameStrBuf, ftime, offset);
                    auto tx = ImGui::CalcTextSize(frameStrBuf.begin(), frameStrBuf.end()).x;
                    uint32_t color = redColor; // (frames.name == 0 && i == 0) ? redColor : activeColor;

                    if (fsz - 5 <= tx)
                    {
                        buf = TimeToString(strBuf, ftime);
                        tx = ImGui::CalcTextSize(strBuf.begin(), strBuf.end()).x;
                    }

                    if (fbegin >= view.zvStartNS_)
                    {
                        draw->AddLine(wpos + ImVec2((fbegin - view.zvStartNS_) * pxns + 2, 1), wpos + ImVec2((fbegin - view.zvStartNS_) * pxns + 2, ty - 1), color);
                    }
                    if (fend <= view.zvEndNS_)
                    {
                        draw->AddLine(wpos + ImVec2((fend - view.zvStartNS_) * pxns - 2, 1), wpos + ImVec2((fend - view.zvStartNS_) * pxns - 2, ty - 1), color);
                    }
                    if (fsz - 5 > tx)
                    {
                        const auto part = (fsz - 5 - tx) / 2;
                        draw->AddLine(wpos + ImVec2(std::max(-10.0, (fbegin - view.zvStartNS_) * pxns + 2), round(ty / 2)), wpos + ImVec2(std::min(w + 20.0, (fbegin - view.zvStartNS_) * pxns + part), round(ty / 2)), color);
                        draw->AddText(wpos + ImVec2((fbegin - view.zvStartNS_) * pxns + 2 + part, 0), color, buf);
                        draw->AddLine(wpos + ImVec2(std::max(-10.0, (fbegin - view.zvStartNS_) * pxns + 2 + part + tx), round(ty / 2)), wpos + ImVec2(std::min(w + 20.0, (fend - view.zvStartNS_) * pxns - 2), round(ty / 2)), color);
                    }
                    else
                    {
                        draw->AddLine(wpos + ImVec2(std::max(-10.0, (fbegin - view.zvStartNS_) * pxns + 2), round(ty / 2)), wpos + ImVec2(std::min(w + 20.0, (fend - view.zvStartNS_) * pxns - 2), round(ty / 2)), color);
                    }
                }
            }
        }
    }

    return hover;
}

const char* ShortenNamespace(const char* name)
{
    return name;
}

uint32_t GetZoneColor(const telemetry::ZoneData& ev)
{
    X_UNUSED(ev);
    return 0xFFCC5555;
}

uint32_t GetZoneHighlight(const telemetry::ZoneData& ev)
{
    const auto color = GetZoneColor(ev);
    return 0xFF000000 |
        (std::min<int>(0xFF, (((color & 0x00FF0000) >> 16) + 25)) << 16) |
        (std::min<int>(0xFF, (((color & 0x0000FF00) >> 8) + 25)) << 8) |
        (std::min<int>(0xFF, (((color & 0x000000FF)) + 25)));
}

int64_t GetZoneEnd(const telemetry::ZoneData& ev)
{
#if 1 // currently we don't support open zones.
    return ev.endNano;
#else
    auto ptr = &ev;
    for (;;)
    {
        if (ptr->endNano >= 0) {
            return ptr->endNano;
        }
        // TODO
       // if (ptr->child < 0) {
            return ptr->startNano;
        //}

//        X_ASSERT_UNREACHABLE();
//        return 0;
    }
#endif
}

float GetZoneThickness(const telemetry::ZoneData& ev)
{
    X_UNUSED(ev);
    return 1.f;
}

const char* GetZoneName(const telemetry::ZoneData& ev)
{
    X_UNUSED(ev);
    return "meooow!";
}


uint32_t DarkenColor(uint32_t color)
{
    return 0xFF000000 |
        (std::min<int>(0xFF, (((color & 0x00FF0000) >> 16) * 2 / 3)) << 16) |
        (std::min<int>(0xFF, (((color & 0x0000FF00) >> 8) * 2 / 3)) << 8) |
        (std::min<int>(0xFF, (((color & 0x000000FF)) * 2 / 3)));
}

int DrawZoneLevel(TraceView& view, ZoneSegmentThread& thread, bool hover, double pxns, const ImVec2& wpos, 
    int _offset, int depth, float yMin, float yMax)
{

    X_UNUSED(yMin, yMax, hover);

    // const auto delay = m_worker.GetDelay();
    // const auto resolution = m_worker.GetResolution();

    auto& zones = thread.zones;

    // find the last zone that ends before view.
    auto it = std::lower_bound(zones.begin(), zones.end(), view.zvStartNS_, [](const auto& l, const auto& r) { return l.endNano < r; });
    if (it == zones.end()) {
        return depth;
    }

    // find the first zone that starts after view.
    const auto zitend = std::lower_bound(it, zones.end(), view.zvEndNS_, [](const auto& l, const auto& r) { return l.startNano < r; });
    if (it == zitend) {
        return depth;
    }
    
#if false
    if ((*it)->end < 0 && m_worker.GetZoneEnd(**it) < view.zvStartNS_) {
        return depth;
    }
#endif

    const auto w = ImGui::GetWindowContentRegionWidth() - 1;
    const auto ty = ImGui::GetFontSize();
    const auto ostep = ty + 1;
    const auto offset = _offset + ostep * depth;
    auto draw = ImGui::GetWindowDrawList();
    const auto dsz = pxns;
    const auto rsz = pxns;

    depth++;
    int maxdepth = depth;

    StringBuf strBuf;

    while (it < zitend)
    {
        auto& zone = *it;

        const auto color = GetZoneColor(zone);
        const auto end = GetZoneEnd(zone);
        const auto zsz = std::max((end - zone.startNano) * pxns, pxns * 0.5);

        if (zsz < MinVisSize)
        {
            int num = 1;
            const auto px0 = (zone.startNano - view.zvStartNS_) * pxns;
            auto px1 = (end - view.zvStartNS_) * pxns;
            auto rend = end;
            for (;;)
            {
                ++it;
                if (it == zitend) {
                    break;
                }

                const auto nend = GetZoneEnd(*it);
                const auto pxnext = (nend - view.zvStartNS_) * pxns;
                if (pxnext - px1 >= MinVisSize * 2) {
                    break;
                }

                px1 = pxnext;
                rend = nend;
                num++;
            }
            
            draw->AddRectFilled(
                wpos + ImVec2(std::max(px0, -10.0), offset), 
                wpos + ImVec2(std::min(std::max(px1, px0 + MinVisSize), double(w + 10)), offset + ty), 
                color);

            DrawZigZag(
                draw, 
                wpos + ImVec2(0, offset + ty / 2), std::max(px0, -10.0), 
                std::min(std::max(px1, px0 + MinVisSize), double(w + 10)), ty / 4, 
                DarkenColor(color));
     
            if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2(std::max(px0, -10.0), offset), wpos + ImVec2(std::min(std::max(px1, px0 + MinVisSize), double(w + 10)), offset + ty)))
            {
                if (num > 1)
                {
                    ImGui::BeginTooltip();
                    TextFocused("Zones too small to display:", IntToString(strBuf, num));
                    ImGui::Separator();
                    TextFocused("Execution time:", TimeToString(strBuf, rend - zone.startNano));
                    ImGui::EndTooltip();

                    if (ImGui::IsMouseClicked(2) && rend - zone.startNano > 0)
                    {
                        ZoomToRange(view, zone.startNano, rend);
                    }
                }
                else
                {
                    ZoneTooltip(view, zone);

                    if (ImGui::IsMouseClicked(2) && rend - zone.startNano > 0)
                    {
                        ZoomToZone(view, zone);
                    }
                    if (ImGui::IsMouseClicked(0))
                    {
                    //    ShowZoneInfo(zone);
                    }
                }
            }

            IntToString(strBuf, num);
            const auto tsz = ImGui::CalcTextSize(strBuf.begin(), strBuf.end());
            if (tsz.x < px1 - px0)
            {
                const auto x = px0 + (px1 - px0 - tsz.x) / 2;
                DrawTextContrast(draw, wpos + ImVec2(x, offset), 0xFF4488DD, strBuf.c_str());
            }
        }
        else
        {
            const char* zoneName = GetZoneName(zone);
            // TODO: 
            int dmul = 1; // zone.text.active ? 2 : 1;

//            bool migration = false;

#if 0

            if (m_lastCpu != zone.cpu_start)
            {
                if (m_lastCpu >= 0)
                {
                    migration = true;
                }
                m_lastCpu = zone.cpu_start;
            }

            if (zone.child >= 0)
            {
                const auto d = DispatchZoneLevel(m_worker.GetZoneChildren(zone.child), hover, pxns, wpos, _offset, depth, yMin, yMax);
                if (d > maxdepth) maxdepth = d;
            }

            if (zone.end >= 0 && m_lastCpu != zone.cpu_end)
            {
                m_lastCpu = zone.cpu_end;
                migration = true;
            }
#endif

            auto tsz = ImGui::CalcTextSize(zoneName);
            if (tsz.x > zsz)
            {
                zoneName = ShortenNamespace(zoneName);
                tsz = ImGui::CalcTextSize(zoneName);
            }

            const auto pr0 = (zone.startNano - view.zvStartNS_) * pxns;
            const auto pr1 = (end - view.zvStartNS_) * pxns;
            const auto px0 = std::max(pr0, -10.0);
            const auto px1 = std::max({ std::min(pr1, double(w + 10)), px0 + pxns * 0.5, px0 + MinVisSize });
            
            draw->AddRectFilled(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y), color);
            draw->AddRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y), GetZoneHighlight(zone), 0.f, -1, GetZoneThickness(zone));
            
            if (dsz * dmul >= MinVisSize)
            {
                draw->AddRectFilled(wpos + ImVec2(pr0, offset), wpos + ImVec2(std::min(pr0 + dsz * dmul, pr1), offset + tsz.y), 0x882222DD);
                draw->AddRectFilled(wpos + ImVec2(pr1, offset), wpos + ImVec2(pr1 + dsz, offset + tsz.y), 0x882222DD);
            }
            if (rsz >= MinVisSize)
            {
                draw->AddLine(wpos + ImVec2(pr0 + rsz, offset + round(tsz.y / 2)), wpos + ImVec2(pr0 - rsz, offset + round(tsz.y / 2)), 0xAAFFFFFF);
                draw->AddLine(wpos + ImVec2(pr0 + rsz, offset + round(tsz.y / 4)), wpos + ImVec2(pr0 + rsz, offset + round(3 * tsz.y / 4)), 0xAAFFFFFF);
                draw->AddLine(wpos + ImVec2(pr0 - rsz, offset + round(tsz.y / 4)), wpos + ImVec2(pr0 - rsz, offset + round(3 * tsz.y / 4)), 0xAAFFFFFF);

                draw->AddLine(wpos + ImVec2(pr1 + rsz, offset + round(tsz.y / 2)), wpos + ImVec2(pr1 - rsz, offset + round(tsz.y / 2)), 0xAAFFFFFF);
                draw->AddLine(wpos + ImVec2(pr1 + rsz, offset + round(tsz.y / 4)), wpos + ImVec2(pr1 + rsz, offset + round(3 * tsz.y / 4)), 0xAAFFFFFF);
                draw->AddLine(wpos + ImVec2(pr1 - rsz, offset + round(tsz.y / 4)), wpos + ImVec2(pr1 - rsz, offset + round(3 * tsz.y / 4)), 0xAAFFFFFF);
            }
            if (tsz.x < zsz)
            {
                const auto x = (zone.startNano - view.zvStartNS_) * pxns + ((end - zone.startNano) * pxns - tsz.x) / 2;
                if (x < 0 || x > w - tsz.x)
                {
                    ImGui::PushClipRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y * 2), true);
                    DrawTextContrast(draw, wpos + ImVec2(std::max(std::max(0., px0), std::min(double(w - tsz.x), x)), offset), 0xFFFFFFFF, zoneName);
                    ImGui::PopClipRect();
                }
                else if (zone.startNano == zone.endNano)
                {
                    DrawTextContrast(draw, wpos + ImVec2(px0 + (px1 - px0 - tsz.x) * 0.5, offset), 0xFFFFFFFF, zoneName);
                }
                else
                {
                    DrawTextContrast(draw, wpos + ImVec2(x, offset), 0xFFFFFFFF, zoneName);
                }
            }
            else
            {
                ImGui::PushClipRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y * 2), true);
                DrawTextContrast(draw, wpos + ImVec2((zone.startNano - view.zvStartNS_) * pxns, offset), 0xFFFFFFFF, zoneName);
                ImGui::PopClipRect();
            }

            if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y)))
            {
                ZoneTooltip(view, zone);

                if (!view.zoomAnim_.active && ImGui::IsMouseClicked(2))
                {
                    ZoomToZone(view, zone);
                }
                if (ImGui::IsMouseClicked(0))
                {
                // open window with more info, like how many goats are in the pen.
                //    ShowZoneInfo(zone);
                }
            }

            ++it;
        }
    }
    return maxdepth;
}

int DispatchZoneLevel(TraceView& view, ZoneSegmentThread& thread, bool hover, double pxns, const ImVec2& wpos, 
    int _offset, int depth, float yMin, float yMax)
{
    const auto ty = ImGui::GetFontSize();
    const auto ostep = ty + 1;
    const auto offset = _offset + ostep * depth;

    const auto yPos = wpos.y + offset;
    if (yPos + ostep >= yMin && yPos <= yMax)
    {
        return DrawZoneLevel(view, thread, hover, pxns, wpos, _offset, depth, yMin, yMax);
    }
    else
    {
        return 0;
    //    return SkipZoneLevel(view, thread, hover, pxns, wpos, _offset, depth, yMin, yMax);
    }
}


// This draws the timeline frame info and zones.
void DrawZones(TraceView& view)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    auto& io = ImGui::GetIO();


    const auto linepos = ImGui::GetCursorScreenPos();
    const auto lineh = ImGui::GetContentRegionAvail().y;

    bool drawMouseLine = DrawZoneFramesHeader(view);

    drawMouseLine |= DrawZoneFrames(view);

    ImGui::BeginChild("##zoneWin", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const auto wpos = ImGui::GetCursorScreenPos();
    const auto w = ImGui::GetWindowContentRegionWidth() - 1;
    const auto h = std::max<float>(view.zvHeight_, ImGui::GetContentRegionAvail().y - 4); // magic border value
    auto draw = ImGui::GetWindowDrawList();

    if (h == 0) {
        ImGui::EndChild();
        return;
    }

    ImGui::InvisibleButton("##zones", ImVec2(w, h));
    bool hover = ImGui::IsItemHovered();

    auto timespan = view.GetVisiableNS();
    auto pxns = w / double(timespan);

    if (hover)
    {
        drawMouseLine = true;
        HandleZoneViewMouse(view, timespan, wpos, w, pxns);
    }

    const auto ty = ImGui::GetFontSize();
    const auto ostep = ty + 1;
    int offset = 0;
    const auto to = 9.f;
    const auto th = (ty - to) * sqrt(3) * 0.5;

    const auto yMin = linepos.y;
    const auto yMax = yMin + lineh;

    // Draw the threads
    if (view.segments.isNotEmpty())
    {
        auto& segment = view.segments.front();

        for (auto& thread : segment.threads)
        {
            X_UNUSED(thread);

            bool expanded = true;

            const auto yPos = wpos.y + offset;
            if (yPos + ostep >= yMin && yPos <= yMax)
            {
                draw->AddLine(wpos + ImVec2(0, offset + ostep - 1), wpos + ImVec2(w, offset + ostep - 1), 0x33FFFFFF);

                const auto labelColor = (expanded ? 0xFFFFFFFF : 0xFF888888);

                if (expanded)
                {
                    draw->AddTriangleFilled(wpos + ImVec2(to / 2, offset + to / 2), wpos + ImVec2(ty - to / 2, offset + to / 2), wpos + ImVec2(ty * 0.5, offset + to / 2 + th), labelColor);
                }
                else
                {
                    draw->AddTriangle(wpos + ImVec2(to / 2, offset + to / 2), wpos + ImVec2(to / 2, offset + ty - to / 2), wpos + ImVec2(to / 2 + th, offset + ty * 0.5), labelColor, 2.0f);

                }

                const char* txt = "Thread";
                const auto txtsz = ImGui::CalcTextSize(txt);

                draw->AddText(wpos + ImVec2(ty, offset), labelColor, txt);

                if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2(0, offset), wpos + ImVec2(ty + txtsz.x, offset + ty)))
                {
                    if (ImGui::IsMouseClicked(0))
                    {
                        expanded = !expanded;
                    }

                    ImGui::BeginTooltip();
#if true
                    ImGui::TextUnformatted("hi :)");
                    ImGui::SameLine();
                    ImGui::TextDisabled("(0x%" PRIx64 ")", thread.id);
#else
                    ImGui::TextUnformatted(m_worker.GetThreadString(v->id));
                    ImGui::SameLine();
                    ImGui::TextDisabled("(0x%" PRIx64 ")", thread.id);
                 
                    if (!v->timeline.empty())
                    {
                        ImGui::Separator();
                        TextFocused("Appeared at", TimeToString(v->timeline.front()->start - m_worker.GetTimeBegin()));
                        TextFocused("Zone count:", RealToString(v->count, true));
                        TextFocused("Top-level zones:", RealToString(v->timeline.size(), true));
                    }
#endif
                    ImGui::EndTooltip();
                }
            }

            offset += ostep;

            if (expanded)
            {
            //    m_lastCpu = -1;

#if 1
                // if (m_drawZones)
                {
                    const auto depth = DispatchZoneLevel(view, thread, hover, pxns, wpos, offset, 0, yMin, yMax);
                    offset += ostep * depth;
                }
#endif

#if 0
                if (m_drawLocks)
                {
                    const auto depth = DrawLocks(v->id, hover, pxns, wpos, offset, nextLockHighlight, yMin, yMax);
                    offset += ostep * depth;
                }
#endif
            }
            offset += ostep * 0.2f;
        }
    }

    const auto scrollPos = ImGui::GetScrollY();
    if (scrollPos == 0 && view.zvScroll_ != 0)
    {
        view.zvHeight_ = 0;
    }
    else
    {
        if (offset > view.zvHeight_) {
            view.zvHeight_ = offset;
        }
    }

    view.zvScroll_ = scrollPos;


    ImGui::EndChild();

    if (view.highlight_.active && view.highlight_.start != view.highlight_.end)
    {
        const auto s = std::min(view.highlight_.start, view.highlight_.end);
        const auto e = std::max(view.highlight_.start, view.highlight_.end);
        draw->AddRectFilled(
            ImVec2(wpos.x + (s - view.zvStartNS_) * pxns, linepos.y), 
            ImVec2(wpos.x + (e - view.zvStartNS_) * pxns, linepos.y + lineh),
            0x228888DD);
        draw->AddRect(
            ImVec2(wpos.x + (s - view.zvStartNS_) * pxns, linepos.y), 
            ImVec2(wpos.x + (e - view.zvStartNS_) * pxns, linepos.y + lineh), 
            0x448888DD);

        StringBuf strBuf;
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(TimeToString(strBuf, e - s));
        ImGui::EndTooltip();
    }
    else if (drawMouseLine)
    {
        draw->AddLine(ImVec2(io.MousePos.x, linepos.y), ImVec2(io.MousePos.x, linepos.y + lineh), 0x33FFFFFF);
    }

    if (view.highlightZoom_.active && view.highlightZoom_.start != view.highlightZoom_.end)
    {
        const auto s = std::min(view.highlightZoom_.start, view.highlightZoom_.end);
        const auto e = std::max(view.highlightZoom_.start, view.highlightZoom_.end);
        draw->AddRectFilled(
            ImVec2(wpos.x + (s - view.zvStartNS_) * pxns, linepos.y), 
            ImVec2(wpos.x + (e - view.zvStartNS_) * pxns, linepos.y + lineh), 
            0x1688DD88);
        draw->AddRect(
            ImVec2(wpos.x + (s - view.zvStartNS_) * pxns, linepos.y), 
            ImVec2(wpos.x + (e - view.zvStartNS_) * pxns, linepos.y + lineh), 
            0x2C88DD88);
    }

}

void DrawFrame(Client& client, float ww, float wh)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(ww, wh));
    //    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));

    static bool show_app_metrics = false;
    static bool show_app_style_editor = false;
    static bool show_app_about = false;

    if (show_app_metrics) { 
        ImGui::ShowMetricsWindow(&show_app_metrics);
    }
    if (show_app_style_editor) { 
        ImGui::Begin("Style Editor", &show_app_style_editor); 
        ImGui::ShowStyleEditor(); 
        ImGui::End(); 
    }
    if (show_app_about) { 
        ImGui::ShowAboutWindow(&show_app_about); 
    }

    ImGui::Begin("##TelemetryViewerMain", nullptr,
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_MenuBar);

    // Menu
    ImVec2 menuBarSize;

    if (ImGui::BeginMainMenuBar())
    {
        menuBarSize = ImGui::GetWindowSize();
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {

            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("Metrics", NULL, &show_app_metrics);
            ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
            ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);
            ImGui::MenuItem("About", NULL, &show_app_about);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    const float h = wh - menuBarSize.y;
    const float defauktWorkspaceWidth = 300.f;
    const float minWorkspaceWidth = 128.f;
    const float minTracesWidth = 256.f;

    static float sz1 = defauktWorkspaceWidth;
    static float sz2 = ww - sz1;

    static float lastWidth = ww;

    if (ww != lastWidth) {

        // keep the sidebar simular size.
        if (ww < sz1) {
            float leftR = sz1 / lastWidth;
            float rightR = sz2 / lastWidth;

            sz1 = ww * leftR;
            sz2 = ww * rightR;
        }
        else {
            sz2 = ww * sz1;
        }

        lastWidth = ww;
    }

    Splitter(true, 4.0f, &sz1, &sz2, minWorkspaceWidth, minTracesWidth);
    {
        ImGui::BeginChild("##1", ImVec2(sz1, -1), false);

        if (ImGui::BeginTabBar("Main Tabs"))
        {
            if (ImGui::BeginTabItem("Traces", nullptr, 0))
            {
               // float infoHeight = 200.f;
                static core::Guid selectGuid;

                ImGui::BeginChild("##TraceList", ImVec2(0, h - 200.f));
                {
                    if (client.isConnected())
                    {
                        core::CriticalSection::ScopedLock lock(client.dataCS);

                        // need to be able to select one but across many.
                        // so it should just be guid.
                        for (const auto& app : client.apps)
                        {
                            if (ImGui::CollapsingHeader(app.appName.c_str()))
                            {
                                // ImGui::Text(app.appName.c_str());
                                // ImGui::Text("Num %" PRIuS, app.traces.size());
                                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.2f, 0.2f, 0.2f));

                                for (int32_t i = 0; i < static_cast<int32_t>(app.traces.size()); i++)
                                {
                                    const auto& trace = app.traces[i % app.traces.size()];

                                    // want to build a string like:
                                    // hostname - 6 min ago
                                    // auto timeNow = core::DateTimeStamp::getSystemDateTime();
                                    ImGui::PushID(i);

                                    core::StackString256 label(trace.hostName.begin(), trace.hostName.end());
                                    label.appendFmt(" - %" PRId32, i);

                                    if (ImGui::Selectable(label.c_str(), trace.guid == selectGuid))
                                    {
                                        selectGuid = trace.guid;

                                        // need to dispatch a request to get the stats.
                                        QueryTraceInfo qti;
                                        qti.dataSize = sizeof(qti);
                                        qti.type = PacketType::QueryTraceInfo;
                                        qti.guid = trace.guid;
                                        client.sendDataToServer(&qti, sizeof(qti));
                                    }

                                    ImGui::PopID();
                                }

                                ImGui::PopStyleColor(1);
                            }
                        }
                    }
                    else
                    {
                        // show some connect button.
                        ImGui::Separator();
                        ImGui::TextUnformatted("Connect to server");

                        const bool connecting = client.conState == Client::ConnectionState::Connecting;

                        if (connecting)
                        {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                        }

                        static char addr[256] = { "127.0.0.1" };
                        bool connectClicked = false;
                        connectClicked |= ImGui::InputText("", addr, sizeof(addr), ImGuiInputTextFlags_EnterReturnsTrue);
                        connectClicked |= ImGui::Button("Connect");

                        if (connecting)
                        {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                        }
                        else if (connectClicked && *addr)
                        {
                            client.addr.set(addr);

                            // how to know connecting?
                            client.connectSignal.raise();
                        }
                    }

                }
                ImGui::EndChild();

                // stats.
                {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.f));
                   // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
                    //ImGui::SetNextWindowPos(ImVec2(0, 0));
                  //  ImGui::SetNextWindowSize(ImVec2(-1, 100));


                    ImGui::BeginChild("##TraceInfo", ImVec2(-1, -1), true);

                    if (selectGuid.isValid())
                    {
                        // need stats for this trace.
                        core::CriticalSection::ScopedLock lock(client.dataCS);

                        auto& ts = client.traceStats;
                        auto it = std::find_if(ts.begin(), ts.end(), [](const GuidTraceStats& lhs) {
                            return selectGuid == lhs.first;
                        });

                        if (it != ts.end())
                        {
                            auto& stats = it->second;

                            core::HumanDuration::Str durStr0;
                            HumanNumber::Str numStr;

                            ImGui::Text("Duration: %s", core::HumanDuration::toStringNano(durStr0, stats.durationNano));
                            ImGui::Text("Zones: %s", HumanNumber::toString(numStr, stats.numZones));
                            ImGui::SameLine();
                            if (ImGui::Button("Open"))
                            {
                                // TODO: check if exsists  then open / focus.
                                OpenTrace ot;
                                ot.dataSize = sizeof(ot);
                                ot.type = PacketType::OpenTrace;
                                ot.guid = it->first;
                                client.sendDataToServer(&ot, sizeof(ot));
                            }

                            ImGui::Text("Allocations: %" PRId64, 0_i64);
                            ImGui::SameLine();
                            ImGui::Button("Open");
                        }
                        else
                        {
                            ImGui::Text("Duration: -");
                            ImGui::Text("Zones: -");
                            ImGui::Text("Allocations: -");
                        }
                    }
                    
                    ImGui::EndChild();

                    ImGui::PopStyleColor();
                 //   ImGui::PopStyleVar();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings", nullptr, 0))
            {

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();
    }
    ImGui::SameLine();
    {
        ImGui::BeginChild("##2", ImVec2(sz2, -1), false);

        if (ImGui::BeginTabBar("View Tabs"))
        {
            core::CriticalSection::ScopedLock lock(client.dataCS);

            for (auto& view : client.views)
            {
                if (ImGui::BeginTabItem(view.tabName.c_str(), &view.open_, 0))
                {
                    DrawFrames(view);
                    DrawZones(view);
                    //    DrawZones();

                    if (view.zoomAnim_.active)
                    {
                        const auto& io = ImGui::GetIO();

                        view.zoomAnim_.progress += io.DeltaTime * view.zoomAnim_.lenMod;
                        if (view.zoomAnim_.progress >= 1.f)
                        {
                            view.zoomAnim_.active = false;
                            view.zvStartNS_ = view.zoomAnim_.start1;
                            view.zvEndNS_ = view.zoomAnim_.end1;
                        }
                        else
                        {
                            const auto v = sqrt(sin(math<double>::HALF_PI * view.zoomAnim_.progress));
                            view.zvStartNS_ = int64_t(view.zoomAnim_.start0 + (view.zoomAnim_.start1 - view.zoomAnim_.start0) * v);
                            view.zvEndNS_ = int64_t(view.zoomAnim_.end0 + (view.zoomAnim_.end1 - view.zoomAnim_.end0) * v);
                        }
                    }


                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}


bool handleTraceZoneSegmentTicks(Client& client, const DataPacketBaseViewer* pBase)
{
    auto* pHdr = static_cast<const ReqTraceZoneSegmentRespTicks*>(pBase);
    if (pHdr->type != DataStreamTypeViewer::TraceZoneSegmentTicks) {
        X_ASSERT_UNREACHABLE();
    }

    // TODO: fix.
    core::CriticalSection::ScopedLock lock(client.dataCS);

    TraceView* pView = client.viewForHandle(pHdr->handle);
    if (!pView) {
        return false;
    }

    auto& view = *pView;

    if (view.segments.isEmpty()) {
        view.segments.emplace_back(g_arena);
    }

    auto& segment = view.segments.front();
    segment.endNano = view.stats.durationNano; // TODO: HACK!

    // meow.
    // so i need to find out what segment to add this to.
    // i think segments where going to be based on camels in the north?
    // think for now it's just oging to be be tick ranges
    // so a segment will cover a number of ticks
    // but that make predition hard? since i need to preload zones based on time not ticks.
    // for now lets make single segment rip.


    // so this is tick info for a zone segment request.
    // it might not be all the data for the request. 
    // but should they all be in order? think so.
    auto* pTicks = reinterpret_cast<const DataPacketTickInfo*>(pHdr + 1);

    auto& ticks = segment.ticks;
    ticks.reserve(ticks.size() + pHdr->num);

    for (int32 i = 0; i < pHdr->num; i++)
    {
        auto& tick = pTicks[i];

        TickData td;
        td.start = tick.start;
        td.end = tick.end;
        td.startNano = tick.startNano;
        td.endNano = tick.endNano;

        ticks.push_back(td);
    }

    return true;
}

bool handleTraceZoneSegmentZones(Client& client, const DataPacketBaseViewer* pBase)
{
    auto* pHdr = static_cast<const ReqTraceZoneSegmentRespZones*>(pBase);
    if (pHdr->type != DataStreamTypeViewer::TraceZoneSegmentZones) {
        X_ASSERT_UNREACHABLE();
    }

    core::CriticalSection::ScopedLock lock(client.dataCS);

    TraceView* pView = client.viewForHandle(pHdr->handle);
    if (!pView) {
        return false;
    }

    auto& view = *pView;

    if (view.segments.isEmpty()) {
        view.segments.emplace_back(g_arena);
        view.segments.front().threads.reserve(12);
        for (auto& thread : view.segments.front().threads) {
            thread.zones.reserve(256'000);
        }
    }

    auto& segment = view.segments.front();
    auto& threads = segment.threads;

    core::FixedArray<uint32_t, 16> threadIDs;

    for (auto& thread : threads)
    {
        threadIDs.push_back(thread.id);
    }

    auto* pZones = reinterpret_cast<const DataPacketZone*>(pHdr + 1);
    for (int32 i = 0; i < pHdr->num; i++)
    {
        auto& zone = pZones[i];

        ZoneData zd;
        // sizeof(zd);
        zd.startTicks = zone.start;
        zd.endTicks = zone.end;
        zd.startNano = view.ticksToNano(zone.start);
        zd.endNano = view.ticksToNano(zone.end);
        zd.lineNo = zone.lineNo;
        zd.strIdxFunction = zone.strIdxFunction;
        zd.strIdxFile = zone.strIdxFile;
        zd.strIdxZone = zone.strIdxZone;
        zd.stackDepth = zone.stackDepth;

        // want a thread
        int32_t t;
        for(t =0; t < threadIDs.size(); t++)
        {
            if (threadIDs[t] == zone.threadID)
            {
                break;
            }
        }

        if (t == threadIDs.size()) {
            threads.emplace_back(zone.threadID, g_arena);
            threadIDs.push_back(zone.threadID);
        }

        auto& thread = threads[t];
        thread.zones.push_back(zd);
    }

    return true;
}

bool handleTraceStringsInfo(Client& client, const DataPacketBaseViewer* pBase)
{
    auto* pHdr = static_cast<const ReqTraceStringsRespInfo*>(pBase);
    if (pHdr->type != DataStreamTypeViewer::TraceStringsInfo) {
        X_ASSERT_UNREACHABLE();
    }

    // shake it.
    core::CriticalSection::ScopedLock lock(client.dataCS);

    TraceView* pView = client.viewForHandle(pHdr->handle);
    if (!pView) {
        return false;
    }

    auto& view = *pView;

    view.strings.init(pHdr->num, pHdr->minId, pHdr->maxId, pHdr->strDataSize);
    return true;
}


bool handleTraceStrings(Client& client, const DataPacketBaseViewer* pBase)
{
    auto* pHdr = static_cast<const ReqTraceStringsResp*>(pBase);
    if (pHdr->type != DataStreamTypeViewer::TraceStrings) {
        X_ASSERT_UNREACHABLE();
    }

    // shake it.
    core::CriticalSection::ScopedLock lock(client.dataCS);

    TraceView* pView = client.viewForHandle(pHdr->handle);
    if (!pView) {
        return false;
    }

    auto& view = *pView;
    auto& strings = view.strings;

    auto* pData = reinterpret_cast<const uint8_t*>(pHdr + 1);
    for (int32_t i = 0; i < pHdr->num; i++)
    {
        auto* pStrHdr = reinterpret_cast<const TraceStringHdr*>(pData);
        auto* pStr = reinterpret_cast<const char*>(pStrHdr + 1);

        // we have the string!
        // push it back and made a pointer.
        strings.addString(pStrHdr->id, pStrHdr->length, pStr);

        pData += (sizeof(*pStrHdr) + pStrHdr->length);
    }

    return true;
}


bool handleDataSream(Client& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const DataStreamHdr*>(pData);
    if (pHdr->type != PacketType::DataStream) {
        X_ASSERT_UNREACHABLE();
    }

    // the data is compressed.
    // decompress it..
    int32_t cmpLen = pHdr->dataSize - sizeof(DataStreamHdr);
    int32_t origLen = pHdr->origSize - sizeof(DataStreamHdr);
    X_UNUSED(cmpLen);

    if (cmpLen == origLen) {
        // uncompressed packets. 
        X_ASSERT_NOT_IMPLEMENTED();
    }

    auto* pDst = &client.cmpRingBuf[client.cmpBufferOffset];

    int32_t cmpLenOut = static_cast<int32_t>(client.lz4DecodeStream.decompressContinue(pHdr + 1, pDst, origLen));
    if (cmpLenOut != cmpLen) {
        X_ERROR("TelemViewer", "Failed to inflate data stream");
        return false;
    }

    client.cmpBufferOffset += origLen;
    if (client.cmpBufferOffset >= (COMPRESSION_RING_BUFFER_SIZE - COMPRESSION_MAX_INPUT_SIZE)) {
        client.cmpBufferOffset = 0;
    }

    for (int32 i = 0; i < origLen; i = origLen) // TODO: handle multiple packets per buf.
    {
        auto* pPacket = reinterpret_cast<const DataPacketBaseViewer*>(&pDst[i]);

        switch (pPacket->type)
        {

            case DataStreamTypeViewer::TraceZoneSegmentTicks:
                return handleTraceZoneSegmentTicks(client, pPacket);
            case DataStreamTypeViewer::TraceZoneSegmentZones:
                return handleTraceZoneSegmentZones(client, pPacket);

            case DataStreamTypeViewer::TraceStringsInfo:
                return handleTraceStringsInfo(client, pPacket);
            case DataStreamTypeViewer::TraceStrings:
                return handleTraceStrings(client, pPacket);

            default:
                X_NO_SWITCH_DEFAULT_ASSERT;
        }
    }

    return true;
}

bool handleAppList(Client& client, uint8_t* pData)
{
    X_UNUSED(client, pData);

    auto* pHdr = reinterpret_cast<const AppsListHdr*>(pData);
    if (pHdr->type != PacketType::AppList) {
        X_ASSERT_UNREACHABLE();
    }


    telemetry::TraceAppArr apps(g_arena);
    apps.reserve(pHdr->num);

    auto* pSrcApp = reinterpret_cast<const AppsListData*>(pHdr + 1);

    for (int32_t i = 0; i < pHdr->num; i++)
    {
        TelemFixedStr name(pSrcApp->appName);
        TraceApp app(name, g_arena);
        app.traces.reserve(pSrcApp->numTraces);

        auto* pTraceData = reinterpret_cast<const AppTraceListData*>(pSrcApp + 1);

        for (int32_t x = 0; x < pSrcApp->numTraces; x++)
        {
            auto& srcTrace = pTraceData[x];

            Trace trace;
            trace.active = srcTrace.active;
            trace.guid = srcTrace.guid;
            trace.ticksPerMicro = srcTrace.ticksPerMicro;
            trace.date = srcTrace.date;
            trace.hostName = srcTrace.hostName;
            trace.buildInfo = srcTrace.buildInfo;

            app.traces.emplace_back(std::move(trace));
        }

        apps.emplace_back(std::move(app));

        pSrcApp = reinterpret_cast<const AppsListData*>(pTraceData + pSrcApp->numTraces);
    }

    core::CriticalSection::ScopedLock lock(client.dataCS);
    client.apps = std::move(apps);
    return true;
}


bool handleQueryTraceInfoResp(Client& client, uint8_t* pData)
{
    X_UNUSED(client, pData);

    auto* pHdr = reinterpret_cast<const QueryTraceInfoResp*>(pData);
    if (pHdr->type != PacketType::QueryTraceInfoResp) {
        X_ASSERT_UNREACHABLE();
    }

    core::CriticalSection::ScopedLock lock(client.dataCS);
    
    // Insert or update.
    auto& ts = client.traceStats;
    auto it = std::find_if(ts.begin(), ts.end(), [pHdr](const GuidTraceStats& lhs) {
        return pHdr->guid == lhs.first;
    });

    if (it != ts.end())
    {
        it->second = pHdr->stats;
    }
    else
    {
        ts.emplace_back(pHdr->guid, pHdr->stats);
    }
    
    return true;
}

bool handleOpenTraceResp(Client& client, uint8_t* pData)
{
    auto* pHdr = reinterpret_cast<const OpenTraceResp*>(pData);
    if (pHdr->type != PacketType::OpenTraceResp) {
        X_ASSERT_UNREACHABLE();
    }

    if (pHdr->handle < 0) {
        X_ERROR("TelemViewer", "Failed to open trace");
        return true;
    }

    core::CriticalSection::ScopedLock lock(client.dataCS);
    client.views.emplace_back(pHdr->guid, pHdr->ticksPerMicro, pHdr->stats, pHdr->handle, g_arena);

    // ask for some data?
    // do i want zones and ticks seperate?
    // think not, we know the total time of the trace.
    // but we only show ticks for current window.
    // so we kinda want data at same time.
    // should i index the zone data based on ticks?

    // in terms of getting the data back i just kinda wanna build a zone so having all the ticks and zones for a 
    // segment would be nice.
    // and the segment size is ajustable.
    // so i think this will have to be time based.
    // the problem is knowing how much data we are going to get back?
    // i guess if we just request small blocks say 10ms.
    // and just keep doing that it be okay.
    // some sort of sliding window kinda thing that works out a good time range to request.

    ReqTraceStrings rts;
    rts.type = PacketType::ReqTraceStrings;
    rts.dataSize = sizeof(rts);
    rts.handle = pHdr->handle;
    client.sendDataToServer(&rts, sizeof(rts));

    ReqTraceZoneSegment rzs;
    rzs.type = PacketType::ReqTraceZoneSegment;
    rzs.dataSize = sizeof(rzs);
    rzs.handle = pHdr->handle;
    // so what should i request here?
    // time segments?
    // auto start = 0;
    // auto mid = pHdr->stats.numTicks / 2;
    // auto trailing = pHdr->stats.numTicks - mid;

    rzs.startNano = 0;
    rzs.endNano = 1000 * 1000 * 1000;
    client.sendDataToServer(&rzs, sizeof(rzs));

#if 0
    rzs.endNano = 1000 * 1000 * 500;
    rzs.endNano = rzs.startNano + 1000 * 1000 * 1000;
    client.sendDataToServer(&rzs, sizeof(rzs));
#endif

    return true;
}


bool processPacket(Client& client, uint8_t* pData)
{
    auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

    switch (pPacketHdr->type)
    {
        case PacketType::ConnectionRequestAccepted: {
            auto* pConAccept = reinterpret_cast<const ConnectionRequestAcceptedHdr*>(pData);
            client.serverVer = pConAccept->serverVer;
            return true;
        }
        case PacketType::ConnectionRequestRejected: {
            auto* pConRej = reinterpret_cast<const ConnectionRequestRejectedHdr*>(pData);
            auto* pStrData = reinterpret_cast<const char*>(pConRej + 1);
            X_ERROR("Telem", "Connection rejected: %.*s", pConRej->reasonLen, pStrData);
            return false;
        }
        case PacketType::DataStream:
            return handleDataSream(client, pData);

        case PacketType::AppList:
            return handleAppList(client, pData);
        case PacketType::QueryTraceInfoResp:
            return handleQueryTraceInfoResp(client, pData);
        case PacketType::OpenTraceResp:
            return handleOpenTraceResp(client, pData);

        default:
            X_ERROR("TelemViewer", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
            return false;
    }
}


void Client::sendDataToServer(const void* pData, int32_t len)
{
#if X_DEBUG
    if (len > MAX_PACKET_SIZE) {
        ::DebugBreak();
    }
#endif // X_DEBUG

    // send some data...
    // TODO: none blocking?
    int res = platform::send(socket, reinterpret_cast<const char*>(pData), len, 0);
    if (res == SOCKET_ERROR) {
        X_ERROR("Telem", "Socket: send failed with error: %d", platform::WSAGetLastError());
        return;
    }
}

bool readPacket(Client& client, char* pBuffer, int& bufLengthInOut)
{
    // this should return complete packets or error.
    int bytesRead = 0;
    int bufLength = sizeof(PacketBase);

    while (1) {
        int maxReadSize = bufLength - bytesRead;
        int res = platform::recv(client.socket, &pBuffer[bytesRead], maxReadSize, 0);

        if (res == 0) {
            X_ERROR("Telem", "Connection closing...");
            return false;
        }
        else if (res < 0) {
            X_ERROR("Telem", "recv failed with error: %d", platform::WSAGetLastError());
            return false;
        }

        bytesRead += res;

        X_LOG0("Telem", "got: %d bytes", res);

        if (bytesRead == sizeof(PacketBase))
        {
            auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
            if (pHdr->dataSize == 0) {
                X_ERROR("Telem", "Client sent packet with length zero...");
                return false;
            }

            if (pHdr->dataSize > bufLengthInOut) {
                X_ERROR("Telem", "Client sent oversied packet of size %i...", static_cast<tt_int32>(pHdr->dataSize));
                return false;
            }

            bufLength = pHdr->dataSize;
        }

        if (bytesRead == bufLength) {
            bufLengthInOut = bytesRead;
            return true;
        }
        else if (bytesRead > bufLength) {
            X_ERROR("Telem", "Overread packet bytesRead: %d recvbuflen: %d", bytesRead, bufLength);
            return false;
        }
    }
}

Client::Client(core::MemoryArenaBase* arena) :
    addr("127.0.0.1"),
    port(8001),
    conState(ConnectionState::Offline),
    connectSignal(true),
    socket(INV_SOCKET),
    cmpBufferOffset(0),
    apps(arena),
    traceStats(arena),
    views(arena)
{
    
}

bool Client::isConnected(void) const 
{
    return socket != INV_SOCKET;
}

TraceView* Client::viewForHandle(tt_int8 handle)
{
    TraceView* pView = nullptr;

    for (auto& view : views)
    {
        if (view.handle == handle)
        {
            return &view;
        }
    }

    return pView;
}


bool connectToServer(Client& client)
{
    if (client.isConnected()) {
        // TODO:
        return false;
    }

    struct platform::addrinfo hints, *servinfo = nullptr;
    core::zero_object(hints);
    hints.ai_family = AF_UNSPEC; // ipv4/6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = platform::IPPROTO_TCP;

    // Resolve the server address and port
    core::StackString<16, char> portStr(client.port);

    auto res = platform::getaddrinfo(client.addr.c_str(), portStr.c_str(), &hints, &servinfo);
    if (res != 0) {
        X_ERROR("Telem", "Failed to get addre info. Error: %d", platform::WSAGetLastError());
        return false;
    }

    platform::SOCKET connectSocket = INV_SOCKET;

    for (auto pPtr = servinfo; pPtr != nullptr; pPtr = pPtr->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = platform::socket(pPtr->ai_family, pPtr->ai_socktype, pPtr->ai_protocol);
        if (connectSocket == INV_SOCKET) {
            return false;
        }

        // Connect to server.
        res = connect(connectSocket, pPtr->ai_addr, static_cast<int>(pPtr->ai_addrlen));
        if (res == SOCKET_ERROR) {
            platform::closesocket(connectSocket);
            connectSocket = INV_SOCKET;
            continue;
        }

        break;
    }

    platform::freeaddrinfo(servinfo);

    if (connectSocket == INV_SOCKET) {
        return false;
    }

    // how big?
    tt_int32 sock_opt = 1024 * 16;
    res = platform::setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        X_ERROR("Telem", "Failed to set sndbuf on socket. Error: %d", platform::WSAGetLastError());
        return false;
    }

    ConnectionRequestViewerHdr cr;
    cr.dataSize = sizeof(cr);
    cr.type = PacketType::ConnectionRequestViewer;
    cr.viewerVer.major = TELEM_VERSION_MAJOR;
    cr.viewerVer.minor = TELEM_VERSION_MINOR;
    cr.viewerVer.patch = TELEM_VERSION_PATCH;
    cr.viewerVer.build = TELEM_VERSION_BUILD;

    client.socket = connectSocket;
    client.sendDataToServer(&cr, sizeof(cr));

    // wait for a response O.O
    char recvbuf[MAX_PACKET_SIZE];
    int recvbuflen = sizeof(recvbuf);

    // TODO: support timeout.
    if (!readPacket(client, recvbuf, recvbuflen)) {
        return false;
    }

    if (!processPacket(client, reinterpret_cast<tt_uint8*>(recvbuf))) {
        return false;
    }

    return true;
}

core::Thread::ReturnValue threadFunc(const core::Thread& thread)
{
    char recvbuf[MAX_PACKET_SIZE];

    Client& client = *reinterpret_cast<Client*>(thread.getData());

    // do work!
    while (thread.shouldRun())
    {
        // we need to wait till we are told to connect.
        // then try connect.
        while (!client.isConnected())
        {
            client.connectSignal.wait();
            client.conState = Client::ConnectionState::Connecting;

            if (!connectToServer(client)) {
                client.conState = Client::ConnectionState::Offline;
            }
        }

        client.conState = Client::ConnectionState::Connected;

        // listen for packets.
        int recvbuflen = sizeof(recvbuf);

        if (!readPacket(client, recvbuf, recvbuflen)) {
            return false;
        }

        if (!processPacket(client, reinterpret_cast<tt_uint8*>(recvbuf))) {
            return false;
        }
    }

    return core::Thread::ReturnValue(0);
}

bool run(Client& client)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        X_ERROR("", "Error: %s", SDL_GetError());
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    auto pWindow = SDL_CreateWindow("TelemetryViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(pWindow);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = gl3wInit() != 0;
    if (err) {
        X_ERROR("Telem", "Failed to initialize OpenGL loader!");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(pWindow, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    const ImVec4 clearColor = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);

    bool done = false;

    // start a thread for the socket.
    core::Thread thread;
    thread.create("Worker", 1024 * 64);
    thread.setData(&client);
    thread.start(threadFunc);

    // try connect to server
    client.connectSignal.raise();

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                done = true;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(pWindow)) {
                done = true;
            }

        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(pWindow);
        ImGui::NewFrame();

        DrawFrame(client, io.DisplaySize.x, io.DisplaySize.y);
     //   ImGui::ShowDemoWindow();

        ImGui::Render();
        SDL_GL_MakeCurrent(pWindow, gl_context);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(pWindow);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();
    return true;
}

X_NAMESPACE_END