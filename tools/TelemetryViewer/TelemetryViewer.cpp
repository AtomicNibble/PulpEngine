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

    const char* RealToString(double val, bool separator)
    {
        enum { Pool = 8 };
        static char bufpool[Pool][64];
        static int bufsel = 0;
        char* buf = bufpool[bufsel];
        bufsel = (bufsel + 1) % Pool;

        sprintf(buf, "%f", val);
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
        return buf;
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

} // namespace


void DrawFrames(TraceView& view)
{
    const auto height = 30 * ImGui::GetTextLineHeight() / 15.f;


    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    auto& io = ImGui::GetIO();

    const auto wpos = ImGui::GetCursorScreenPos();
    const auto wspace = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
    const auto w = wspace.x;
    auto draw = ImGui::GetWindowDrawList();

    ImGui::InvisibleButton("##frames", ImVec2(w, height));
    bool frameRectHover = ImGui::IsItemHovered();

    // Background
    draw->AddRectFilled(wpos, wpos + ImVec2(w, height), 0x33FFFFFF);

    const auto wheel = io.MouseWheel;
    const auto prevScale = view.frameScale_;
    if (frameRectHover)
    {
        if (wheel > 0)
        {
            auto visible = view.GetVisiableNS();
            auto remove = visible / 8;

            view.zvStart_ += remove;
            view.zvEnd_ -= remove;

            if (view.zvStart_ >= view.zvEnd_) {
                view.zvStart_ = view.zvEnd_ - 1;
            }

            if (view.frameScale_ >= 0) {
                view.frameScale_--;
            }
        }
        else if (wheel < 0)
        {
            // we want to zoom out.
            // we just scale hte time range.
            // ideally we take into account the cursor position for zooming
            // or just for zoom in?
            auto visible = view.GetVisiableNS();
            auto add = visible / 2;

            view.zvStart_ -= core::Min(view.zvStart_, add);
            view.zvEnd_  += add;

            if (view.frameScale_ < 10) {
                view.frameScale_++;
            }
        }
    }

    const int32_t fwidth = GetFrameWidth(view.frameScale_);
    // this is how many frames per group.
    // don't think i want it to work that way
    // i also want to support single run apps like converters.
    // which don't really have a frame.
    // i want to do it based on time.
    // so there will be a timescale.
    // i dunno what the default should be like you open a trace and the first 1 second is visible?
    // and you can zoom either in or out?
    // should it be based on width.
    // so if you resize you see same data but bigger.
    // 

    const int32_t group = GetFrameGroup(view.frameScale_);
    const int32_t total = view.stats.numZones;
    const int32_t onScreen = static_cast<int32_t>((w - 2) / fwidth);

    // need to draw the ticks.
    // the viewer is a sliding window.
    // so we need to know out timeoffset in the trace.
    // then find the various segments for drawing.
    // so first thing is to work out what the visible time range is.
    // lets just make it 5 seconds?
    // i also need to show tick info tho.
    // they should just appear on the timeline.
    // so lets just draw a fucking timeline
    // and put in the tick info on top.

    auto timespan = view.GetVisiableNS();
    auto pxns = w / double(timespan);

    const auto ty = ImGui::GetFontSize();

    // Draw the time bar
    {
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

                    if (t >= 0) // prefix the shit.
                    {
                        StringBuf strBuf1;
                        strBuf1.setFmt("+%s", strBuf.c_str());
                        strBuf = strBuf1;
                    }

                    draw->AddText(wpos + ImVec2(x, math<double>::round(ty * 0.5)), 0x66FFFFFF, strBuf.begin(), strBuf.end());
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
    }

    // i want to draw frame markers.
    // we need to just look that what ticks we have and draw them :D
    // i need to come up with a nice structure for storing these linked list maybe?
    // basically want to be able to add to tail and end.
    // but lets just deal with single range for now.
    if(view.segments.isNotEmpty())
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
            while (it != ticks.end() && it->endNano < view.zvEnd_)
            {
                auto& tick = *it;

                // need to find out offset to draw this bitch.
                auto offset = tick.endNano - view.zvStart_;
                auto width = pxns * double(offset);

                draw->AddLine(wpos + ImVec2(width, 0), wpos + ImVec2(width, math<double>::round(ty * 0.5)), 0x66FF00FF);

                ++it;
            }

            // draw->AddRectFilled(wpos, wpos + ImVec2(w, height), 0x33FFFFFF);

        }
    }

    bool drawMouseLine = true;

    if (drawMouseLine)
    {
        const auto linepos = ImGui::GetCursorScreenPos();
        const auto lineh = ImGui::GetContentRegionAvail().y;

        draw->AddLine(ImVec2(io.MousePos.x, linepos.y), ImVec2(io.MousePos.x, linepos.y + lineh), 0x33FFFFFF);
    }

#if 0
    if (!view.paused_)
    {
        view.frameStart_ = (total < onScreen * group) ? 0 : total - onScreen * group;
        SetViewToLastFrames();
    }
#endif

#if 0
    if (hover)
    {
        if (ImGui::IsMouseDragging(1, 0))
        {
            view.paused_ = true;
            const auto delta = ImGui::GetMouseDragDelta(1, 0).x;
            if (abs(delta) >= fwidth)
            {
                const auto d = (int)delta / fwidth;
                view.frameStart_ = std::max(0, view.frameStart_ - d * group);
                io.MouseClickedPos[1].x = io.MousePos.x + d * fwidth - delta;
            }
        }

        const auto mx = io.MousePos.x;
        if (mx > wpos.x && mx < wpos.x + w - 1)
        {
            const auto mo = mx - (wpos.x + 1);
            const auto off = mo * group / fwidth;

            const int sel = view.frameStart_ + off;
            if (sel < total)
            {
                ImGui::BeginTooltip();
                if (group > 1)
                {
                    auto f = GetFrameTime(sel);
                    auto g = std::min(group, total - sel);
                    for (int j = 1; j < g; j++)
                    {
                        f = std::max(f, GetFrameTime(sel + j));
                    }

                    TextDisabledUnformatted("Frames:");
                    ImGui::SameLine();
                    ImGui::Text("%s - %s (%s)", RealToString(sel, true), RealToString(sel + g - 1, true), RealToString(g, true));
                    ImGui::Separator();
                    TextFocused("Max frame time:", TimeToString(f));
                }
                else
                {
                    // TODO:
                    // if (m_frames->name == 0)
                    if (true)
                    {
                        const auto offset = GetFrameOffset();
                        if (sel == 0)
                        {
                            ImGui::TextUnformatted("Tracy initialization");
                            ImGui::Separator();
                            TextFocused("Time:", TimeToString(GetFrameTime(sel)));
                        }
                        else if (offset == 0)
                        {
                            TextDisabledUnformatted("Frame:");
                            ImGui::SameLine();
                            ImGui::TextUnformatted(RealToString(sel, true));
                            ImGui::Separator();
                            TextFocused("Frame time:", TimeToString(GetFrameTime(sel)));
                        }
                        else if (sel == 1)
                        {
                            ImGui::TextUnformatted("Missed frames");
                            ImGui::Separator();
                            TextFocused("Time:", TimeToString(GetFrameTime(1)));
                        }
                        else
                        {
                            TextDisabledUnformatted("Frame:");
                            ImGui::SameLine();
                            ImGui::TextUnformatted(RealToString(sel + offset - 1, true));
                            ImGui::Separator();
                            TextFocused("Frame time:", TimeToString(GetFrameTime(sel)));
                        }
                    }
                    else
                    {
                        //   ImGui::TextDisabled("%s:", GetString(m_frames->name));
                        ImGui::SameLine();
                        ImGui::TextUnformatted(RealToString(sel + 1, true));
                        ImGui::Separator();
                        TextFocused("Frame time:", TimeToString(GetFrameTime(sel)));
                    }
                }
                TextFocused("Time from start of program:", TimeToString(GetFrameBegin(sel) - GetTimeBegin()));
                ImGui::EndTooltip();

                if (ImGui::IsMouseClicked(0))
                {
                    view.paused_ = true;
                    view.zvStart_ = GetFrameBegin(sel);
                    view.zvEnd_ = GetFrameEnd(sel + group - 1);
                    if (view.zvStart_ == view.zvEnd_) {
                        view.zvStart_--;
                    }
                }
                else if (ImGui::IsMouseDragging(0))
                {
                    view.zvStart_ = std::min(view.zvStart_, GetFrameBegin(sel));
                    view.zvEnd_ = std::max(view.zvEnd_, GetFrameEnd(sel + group - 1));
                }
            }

            if (view.paused_ && wheel != 0)
            {
                const int pfwidth = GetFrameWidth(prevScale);
                const int pgroup = GetFrameGroup(prevScale);

                const auto oldoff = mo * pgroup / pfwidth;
                view.frameStart_ = std::min(total, std::max(0, view.frameStart_ - int(off - oldoff)));
            }
        }
    }

#endif

#if 0
    // this draws the zones.
    // but i need to be able to work in segments.
    // and we don't really want segment start / end.
    // so it should be like segment + segment offset.

    int32_t i = 0;
    int32_t idx = 0;
    while (i < onScreen && view.frameStart_ + idx < total)
    {
        auto f = GetFrameTime(view.frameStart_ + idx);

        int32_t g;
        if (group > 1)
        {
            g = std::min(group, total - (view.frameStart_ + idx));
            for (int32_t j = 1; j < g; j++)
            {
                f = std::max(f, GetFrameTime(view.frameStart_ + idx + j));
            }
        }

        X_DISABLE_WARNING(4244)

        const auto h = float(std::min<uint64_t>(MaxFrameTime, f)) / MaxFrameTime * (Height - 2);
        if (fwidth != 1)
        {
            draw->AddRectFilled(
                wpos + ImVec2(1 + i * fwidth, Height - 1 - h),
                wpos + ImVec2(fwidth + i * fwidth, Height - 1),
                GetFrameColor(f)
            );
        }
        else
        {
            draw->AddLine(wpos + ImVec2(1 + i, Height - 2 - h), wpos + ImVec2(1 + i, Height - 2), GetFrameColor(f));
        }

        i++;
        idx += group;
    }
#endif

#if 0
    const std::pair<int, int> zrange = GetFrameRange(view.zvStart_, view.zvEnd_);

    if (zrange.second > view.frameStart_ && zrange.first < view.frameStart_ + onScreen * group)
    {
        auto x1 = std::min(onScreen * fwidth, (zrange.second - view.frameStart_) * fwidth / group);
        auto x0 = std::max(0, (zrange.first - view.frameStart_) * fwidth / group);

        if (x0 == x1) {
            x1 = x0 + 1;
        }

        draw->AddRectFilled(wpos + ImVec2(1 + x0, 0), wpos + ImVec2(1 + x1, Height), 0x55DD22DD);
    }
#endif
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

                                for (int32_t i = 0; i < static_cast<int32_t>(app.traces.size()) * 10; i++)
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
                    //    DrawZones();

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

    TraceView* pView = nullptr;

    for (auto& view : client.views)
    {
        if (view.handle == pHdr->handle)
        {
            pView = &view;
            break;
        }
    }

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
    auto* pHdr = static_cast<const ReqTraceZoneSegmentRespTicks*>(pBase);
    if (pHdr->type != DataStreamTypeViewer::TraceZoneSegmentZones) {
        X_ASSERT_UNREACHABLE();
    }

    X_UNUSED(client);

    pHdr->handle;
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
    client.views.emplace_back(pHdr->guid, pHdr->stats, pHdr->handle, g_arena);

    // ask for some data?
    // do i want zones and ticks seperate?
    // think not, we know the total time of the trace.
    // but we only show ticks for current window.
    // so we kinda want data at same time.
    // should i index the zone data based on ticks?

    // in terms of getting the data back i just kinda wanna build a zone so having all the ticks and zones for a 
    // egment would be nice.
    // and the segment size is ajustable.
    // oh but i was gonna support the sserver just sending you data.
    // so i would need to build segments for that.
    // can i just put data in to segemts?
    // would mean i have to process every block check it's time and place it in a zone.
    // if the zones are out of order could be messy.
    // i was going to get the server to sort zones before insert.
    // so maybe the server should do that and send the data as a segment.
    // acutally the server just relaying you data is probs not a great idea
    // since if you want to look at stuff we don't want anymore data other than new total time to update scroll bar.
    ReqTraceZoneSegment rzs;
    rzs.type = PacketType::ReqTraceZoneSegment;
    rzs.dataSize = sizeof(rzs);
    rzs.handle = pHdr->handle;
    // so what should i request here?
    // time segments?
    auto start = 0;
    auto mid = pHdr->stats.numTicks / 2;
    auto trailing = pHdr->stats.numTicks - mid;

    rzs.tickIdx = start;
    rzs.max = mid;
    client.sendDataToServer(&rzs, sizeof(rzs));

    // humm maybe do this based on ticks?
    // but i dunno how big a tick will be
    rzs.tickIdx = mid;
    rzs.max = trailing;
    client.sendDataToServer(&rzs, sizeof(rzs));

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
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

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