#include "stdafx.h"
#include "EngineApp.h"

#include "TelemetryViewer.h"

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_FORCE_LINK_FACTORY("XTelemSrvLib");

#endif // !X_LIB


X_DISABLE_WARNING(4505) // unreferenced local function has been removed

namespace
{
    typedef core::MemoryArena<
        core::MallocFreeAllocator,
        core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
    >
        TelemetryViewerArena;

    TelemetryViewerArena* g_arena = nullptr;

    bool winSockInit(void)
    {
        platform::WSADATA winsockInfo;

        if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
            return false;
        }

        return true;
    }

    void winSockShutDown(void)
    {
        if (platform::WSACleanup() != 0) {
            // rip
            return;
        }
    }

#if 0

    TraceView view;

    static void DrawFrames()
    {
        const auto Height = 40 * ImGui::GetTextLineHeight() / 15.f;

        enum
        {
            MaxFrameTime = 50 * 1000 * 1000
        }; // 50ms

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) {
            return;
        }

        auto& io = ImGui::GetIO();

        const auto wpos = ImGui::GetCursorScreenPos();
        const auto wspace = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
        const auto w = wspace.x;
        auto draw = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##frames", ImVec2(w, Height));
        bool hover = ImGui::IsItemHovered();

        draw->AddRectFilled(wpos, wpos + ImVec2(w, Height), 0x33FFFFFF);
        const auto wheel = io.MouseWheel;
        const auto prevScale = view.frameScale_;
        if (hover)
        {
            if (wheel > 0)
            {
                if (view.frameScale_ >= 0) {
                    view.frameScale_--;
                }
            }
            else if (wheel < 0)
            {
                if (view.frameScale_ < 10) {
                    view.frameScale_++;
                }
            }
        }

        const int32_t fwidth = GetFrameWidth(view.frameScale_);
        const int32_t group = GetFrameGroup(view.frameScale_);
        const int32_t total = view.numFrames_;
        const int32_t onScreen = static_cast<int32_t>((w - 2) / fwidth);

#if 0
        if (!view.paused_)
        {
            view.frameStart_ = (total < onScreen * group) ? 0 : total - onScreen * group;
            SetViewToLastFrames();
        }
#endif

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
                        if(true)
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

        int32_t i = 0;
        int32_t idx = 0;
        while (i < onScreen && view.frameStart_ + idx < total)
        {
            auto f = GetFrameTime(view.frameStart_ + idx);

            int32_t g;
            if (group > 1)
            {
                g = std::min(group, total - (view.frameStart_ + idx));
                for (int32_t  j = 1; j < g; j++)
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

        const std::pair<int, int> zrange = GetFrameRange(view.zvStart_, view.zvEnd_);

        if (zrange.second > view.frameStart_ && zrange.first < view.frameStart_ + onScreen * group)
        {
            auto x1 = std::min(onScreen * fwidth, (zrange.second - view.frameStart_) * fwidth / group);
            auto x0 = std::max(0, (zrange.first - view.frameStart_) * fwidth / group);

            if (x0 == x1) x1 = x0 + 1;

            draw->AddRectFilled(wpos + ImVec2(1 + x0, 0), wpos + ImVec2(1 + x1, Height), 0x55DD22DD);
        }
    }


    void ZoomToRange(int64_t start, int64_t end)
    {
        if (start == end)
        {
            end = start + 1;
        }

        view.paused_ = true;
        view.highlightZoom_.active = false;
        view.zoomAnim_.active = true;
        view.zoomAnim_.start0 = view.zvStart_;
        view.zoomAnim_.start1 = start;
        view.zoomAnim_.end0 = view.zvEnd_;
        view.zoomAnim_.end1 = end;
        view.zoomAnim_.progress = 0;

        const auto d0 = double(view.zoomAnim_.end0 - view.zoomAnim_.start0);
        const auto d1 = double(view.zoomAnim_.end1 - view.zoomAnim_.start1);
        const auto diff = d0 > d1 ? d0 / d1 : d1 / d0;
        view.zoomAnim_.lenMod = 10.0 / log10(diff);
    }


    void HandleZoneViewMouse(int64_t timespan, const ImVec2& wpos, float w, double& pxns)
    {
        assert(timespan > 0);
        auto& io = ImGui::GetIO();

        const auto nspx = double(timespan) / w;

        if (ImGui::IsMouseClicked(0))
        {
            view.highlight_.active = true;
            view.highlight_.start = view.highlight_.end = view.zvStart_ + (io.MousePos.x - wpos.x) * nspx;
        }
        else if (ImGui::IsMouseDragging(0, 0))
        {
            view.highlight_.end = view.zvStart_ + (io.MousePos.x - wpos.x) * nspx;
        }
        else
        {
            view.highlight_.active = false;
        }

        if (ImGui::IsMouseClicked(2))
        {
            view.highlightZoom_.active = true;
            view.highlightZoom_.start = view.highlightZoom_.end = view.zvStart_ + (io.MousePos.x - wpos.x) * nspx;
        }
        else if (ImGui::IsMouseDragging(2, 0))
        {
            view.highlightZoom_.end = view.zvStart_ + (io.MousePos.x - wpos.x) * nspx;
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
                    const auto tsOld = view.zvEnd_ - view.zvStart_;
                    const auto tsNew = e - s;
                    const auto mul = double(tsOld) / tsNew;
                    const auto left = s - view.zvStart_;
                    const auto right = view.zvEnd_ - e;

                    ZoomToRange(view.zvStart_ - left * mul, view.zvEnd_ + right * mul);
                }
                else
                {
                    ZoomToRange(s, e);
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
                view.zvStart_ -= dpx;
                view.zvEnd_ -= dpx;
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
                view.zvStart_ += int64_t(p1 * 0.25);
                view.zvEnd_ -= int64_t(p2 * 0.25);
            }
            else if (timespan < 1000ll * 1000 * 1000 * 60 * 60)
            {
                view.zvStart_ -= std::max(int64_t(1), int64_t(p1 * 0.25));
                view.zvEnd_ += std::max(int64_t(1), int64_t(p2 * 0.25));
            }
            timespan = view.zvEnd_ - view.zvStart_;
            pxns = w / double(timespan);
        }
    }

    bool DrawZoneFramesHeader()
    {
        const auto wpos = ImGui::GetCursorScreenPos();
        const auto w = ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ScrollbarSize;
        auto draw = ImGui::GetWindowDrawList();
        const auto ty = ImGui::GetFontSize();

        ImGui::InvisibleButton("##zoneFrames", ImVec2(w, ty * 1.5));
        bool hover = ImGui::IsItemHovered();

        auto timespan = view.zvEnd_ - view.zvStart_;
        auto pxns = w / double(timespan);

        if (hover) {
            HandleZoneViewMouse(timespan, wpos, w, pxns);
        }

        {
            const auto nspx = 1.0 / pxns;
            const auto scale = std::max(0.0, round(log10(nspx) + 2));
            const auto step = pow(10, scale);

            const auto dx = step * pxns;
            double x = 0;
            int tw = 0;
            int tx = 0;
            int64_t tt = 0;
            while (x < w)
            {
                draw->AddLine(wpos + ImVec2(x, 0), wpos + ImVec2(x, round(ty * 0.5)), 0x66FFFFFF);
                if (tw == 0)
                {
                    char buf[128];
                    const auto t = view.zvStart_ - GetTimeBegin();
                    auto txt = TimeToString(t);
                    if (t >= 0)
                    {
                        sprintf(buf, "+%s", txt);
                        txt = buf;
                    }
                    draw->AddText(wpos + ImVec2(x, round(ty * 0.5)), 0x66FFFFFF, txt);
                    tw = ImGui::CalcTextSize(txt).x;
                }
                else if (x > tx + tw + ty * 2)
                {
                    tx = x;
                    auto txt = TimeToString(tt);
                    draw->AddText(wpos + ImVec2(x, round(ty * 0.5)), 0x66FFFFFF, txt);
                    tw = ImGui::CalcTextSize(txt).x;
                }

                if (scale != 0)
                {
                    for (int i = 1; i < 5; i++)
                    {
                        draw->AddLine(wpos + ImVec2(x + i * dx / 10, 0), wpos + ImVec2(x + i * dx / 10, round(ty * 0.25)), 0x33FFFFFF);
                    }
                    draw->AddLine(wpos + ImVec2(x + 5 * dx / 10, 0), wpos + ImVec2(x + 5 * dx / 10, round(ty * 0.375)), 0x33FFFFFF);
                    for (int i = 6; i < 10; i++)
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

    const char* ShortenNamespace(const char* name)
    {
        return name;
    }


    uint32_t GetZoneColor(const telemetry::TraceZone& ev)
    {
        X_UNUSED(ev);
        return 0xFFCC5555;
    }

    uint32_t GetZoneHighlight(const telemetry::TraceZone& ev)
    {

            const auto color = GetZoneColor(ev);
            return 0xFF000000 |
                (std::min<int>(0xFF, (((color & 0x00FF0000) >> 16) + 25)) << 16) |
                (std::min<int>(0xFF, (((color & 0x0000FF00) >> 8) + 25)) << 8) |
                (std::min<int>(0xFF, (((color & 0x000000FF)) + 25)));
    }

    int64_t GetZoneEnd(const telemetry::TraceZone& ev)
    {
        auto ptr = &ev;
        for (;;)
        {
            if (ptr->end >= 0) {
                return ptr->end;
            }
            if (ptr->child < 0) {
                return ptr->start;
            }
            
            X_ASSERT_UNREACHABLE();
            return 0;
        }
    }

    float GetZoneThickness(const telemetry::TraceZone& ev)
    {
        X_UNUSED(ev);
        return 1.f;
    }

    const char* GetZoneName(const telemetry::TraceZone& ev)
    {
        X_UNUSED(ev);
        return "meooow!";
    }


    enum { MinVisSize = 3 };
    enum { MinFrameSize = 5 };

    int64_t GetDelay() { return 0; }
    int64_t GetResolution() { return 0; }

    int DrawZoneLevel(const core::Array<telemetry::TraceZone>& zones, bool hover, double pxns, const ImVec2& wpos,
        int _offset, int depth, float yMin, float yMax)
    {
        X_UNUSED(yMin, yMax);

        const auto delay = GetDelay();
        const auto resolution = GetResolution();
        // cast to uint64_t, so that unended zones (end = -1) are still drawn
        auto it = std::lower_bound(zones.begin(), zones.end(), view.zvStart_ - delay, [](const auto& l, const auto& r) { return (uint64_t)l.end < (uint64_t)r; });
        if (it == zones.end()) {
            return depth;
        }

        const auto zitend = std::lower_bound(it, zones.end(), view.zvEnd_ + resolution, [](const auto& l, const auto& r) { return l.start < r; });
        if (it == zitend) {
            return depth;
        }

        if (it->end < 0 && GetZoneEnd(*it) < view.zvStart_) {
            return depth;
        }

        const auto w = ImGui::GetWindowContentRegionWidth() - 1;
        const auto ty = ImGui::GetFontSize();
        const auto ostep = ty + 1;
        const auto offset = _offset + ostep * depth;
        auto draw = ImGui::GetWindowDrawList();
        const auto dsz = delay * pxns;
        const auto rsz = resolution * pxns;

        depth++;
        int maxdepth = depth;

        while (it < zitend)
        {
            auto& ev = *it;
            const auto color = GetZoneColor(ev);
            const auto end = GetZoneEnd(ev);
            const auto zsz = std::max((end - ev.start) * pxns, pxns * 0.5);
            if (zsz < MinVisSize)
            {
                int num = 1;
                const auto px0 = (ev.start - view.zvStart_) * pxns;
                auto px1 = (end - view.zvStart_) * pxns;
                auto rend = end;
                for (;;)
                {
                    ++it;
                    if (it == zitend) break;
                    const auto nend = GetZoneEnd(*it);
                    const auto pxnext = (nend - view.zvStart_) * pxns;
                    if (pxnext - px1 >= MinVisSize * 2) break;
                    px1 = pxnext;
                    rend = nend;
                    num++;
                }
                draw->AddRectFilled(wpos + ImVec2(std::max(px0, -10.0), offset), wpos + ImVec2(std::min(std::max(px1, px0 + MinVisSize), double(w + 10)), offset + ty), color);
             //   DrawZigZag(draw, wpos + ImVec2(0, offset + ty / 2), std::max(px0, -10.0), std::min(std::max(px1, px0 + MinVisSize), double(w + 10)), ty / 4, DarkenColor(color));
                if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2(std::max(px0, -10.0), offset), wpos + ImVec2(std::min(std::max(px1, px0 + MinVisSize), double(w + 10)), offset + ty)))
                {
                    if (num > 1)
                    {
                        ImGui::BeginTooltip();
                        TextFocused("Zones too small to display:", RealToString(num, true));
                        ImGui::Separator();
                        TextFocused("Execution time:", TimeToString(rend - ev.start));
                        ImGui::EndTooltip();

                        if (ImGui::IsMouseClicked(2) && rend - ev.start > 0)
                        {
                            ZoomToRange(ev.start, rend);
                        }
                    }
                    else
                    {
#if false
                        ZoneTooltip(ev);

                        if (ImGui::IsMouseClicked(2) && rend - ev.start > 0)
                        {
                            ZoomToZone(ev);
                        }
                        if (ImGui::IsMouseClicked(0))
                        {
                            ShowZoneInfo(ev);
                        }

                        m_zoneSrcLocHighlight = ev.srcloc;
#endif
                    }
                }
                char tmp[64];
                sprintf(tmp, "%s", RealToString(num, true));
                const auto tsz = ImGui::CalcTextSize(tmp);
                if (tsz.x < px1 - px0)
                {
                    const auto x = px0 + (px1 - px0 - tsz.x) / 2;
                    DrawTextContrast(draw, wpos + ImVec2(x, offset), 0xFF4488DD, tmp);
                }
            }
            else
            {
#if true
                const char* zoneName = GetZoneName(ev);
                int dmul = 1; // ev.text.active ? 2 : 1;


#if false
                if (ev.child >= 0)
                {
                    const auto d = DispatchZoneLevel(GetZoneChildren(ev.child), hover, pxns, wpos, _offset, depth, yMin, yMax);
                    if (d > maxdepth) {
                        maxdepth = d;
                    }
                }
#endif

                auto tsz = ImGui::CalcTextSize(zoneName);
                if (tsz.x > zsz)
                {
                    zoneName = ShortenNamespace(zoneName);
                    tsz = ImGui::CalcTextSize(zoneName);
                }

                const auto pr0 = (ev.start - view.zvStart_) * pxns;
                const auto pr1 = (end - view.zvStart_) * pxns;
                const auto px0 = std::max(pr0, -10.0);
                const auto px1 = std::max({ std::min(pr1, double(w + 10)), px0 + pxns * 0.5, px0 + MinVisSize });
                draw->AddRectFilled(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y), color);
                draw->AddRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y), GetZoneHighlight(ev), 0.f, -1, GetZoneThickness(ev));
                
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
                    const auto x = (ev.start - view.zvStart_) * pxns + ((end - ev.start) * pxns - tsz.x) / 2;
                    if (x < 0 || x > w - tsz.x)
                    {
                        ImGui::PushClipRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y * 2), true);
                        DrawTextContrast(draw, wpos + ImVec2(std::max(std::max(0., px0), std::min(double(w - tsz.x), x)), offset), 0xFFFFFFFF, zoneName);
                        ImGui::PopClipRect();
                    }
                    else if (ev.start == ev.end)
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
                    DrawTextContrast(draw, wpos + ImVec2((ev.start - view.zvStart_) * pxns, offset), 0xFFFFFFFF, zoneName);
                    ImGui::PopClipRect();
                }

#if false
                if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2(px0, offset), wpos + ImVec2(px1, offset + tsz.y)))
                {
                    ZoneTooltip(ev);

                    if (!m_zoomAnim.active && ImGui::IsMouseClicked(2))
                    {
                        ZoomToZone(ev);
                    }
                    if (ImGui::IsMouseClicked(0))
                    {
                        ShowZoneInfo(ev);
                    }

                    m_zoneSrcLocHighlight = ev.srcloc;
                }
#endif
#endif

                ++it;
            }
        }
        return maxdepth;
    }


    int DispatchZoneLevel(const core::Array<telemetry::TraceZone>& zones, bool hover, double pxns, const ImVec2& wpos, int _offset, int depth, float yMin, float yMax)
    {
        const auto ty = ImGui::GetFontSize();
        const auto ostep = ty + 1;
        const auto offset = _offset + ostep * depth;

        const auto yPos = wpos.y + offset;
        if (yPos + ostep >= yMin && yPos <= yMax)
        {
            return DrawZoneLevel(zones, hover, pxns, wpos, _offset, depth, yMin, yMax);
        }
        else
        {
            return 1;
        //    return SkipZoneLevel(vec, hover, pxns, wpos, _offset, depth, yMin, yMax);
        }
    }


    static void DrawZones()
    {
        const auto linepos = ImGui::GetCursorScreenPos();
        const auto lineh = ImGui::GetContentRegionAvail().y;

        bool drawMouseLine = DrawZoneFramesHeader();
        
#if false
        auto& frames = GetFrames();
        for (auto fd : frames)
        {
            if (Visible(fd))
            {
                drawMouseLine |= DrawZoneFrames(*fd);
            }
        }
#endif

        ImGui::BeginChild("##zoneWin", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const auto wpos = ImGui::GetCursorScreenPos();
        const auto w = ImGui::GetWindowContentRegionWidth() - 1;
        const auto h = std::max<float>(view.zvHeight_, ImGui::GetContentRegionAvail().y - 4);    // magic border value
        auto draw = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##zones", ImVec2(w, h));
        bool hover = ImGui::IsItemHovered();

        const auto timespan = view.zvEnd_ - view.zvStart_;
        auto pxns = w / double(timespan);

        if (hover)
        {
            drawMouseLine = true;
            HandleZoneViewMouse(timespan, wpos, w, pxns);
        }

        const auto nspx = 1.0 / pxns;

        const auto ty = ImGui::GetFontSize();
        const auto ostep = ty + 1;
        int offset = 0;
        const auto to = 9.f;
        const auto th = (ty - to) * sqrt(3) * 0.5;

        const auto yMin = linepos.y;
        const auto yMax = yMin + lineh;

        // Zones
        core::Array<telemetry::TraceThread> threads(g_arena);

        for (size_t t = 0; t < 10; t++)
        {
            auto& thread = threads.AddOne(g_arena);

            for (size_t i = 0; i < 100; i++)
            {
                thread.zones.emplace_back(i * 100, (i * 100) + 50);
            }
        }

        for (const auto& v : threads)
        {
            // bool& showFull = ShowFull(v);
            bool showFull = true;

            const auto yPos = wpos.y + offset;
            if (yPos + ostep >= yMin && yPos <= yMax)
            {
                draw->AddLine(wpos + ImVec2(0, offset + ostep - 1), wpos + ImVec2(w, offset + ostep - 1), 0x33FFFFFF);

                const auto labelColor =  (showFull ? 0xFFFFFFFF : 0xFF888888);

                {
                    draw->AddTriangle(wpos + ImVec2(to / 2, offset + to / 2), wpos + ImVec2(to / 2, offset + ty - to / 2), wpos + ImVec2(to / 2 + th, offset + ty * 0.5), labelColor, 2.0f);
                }

                const auto txt = GetThreadString(v.id);
                const auto txtsz = ImGui::CalcTextSize(txt);

                draw->AddText(wpos + ImVec2(ty, offset), labelColor, txt);

                if (hover && ImGui::IsMouseHoveringRect(wpos + ImVec2(0, offset), wpos + ImVec2(ty + txtsz.x, offset + ty)))
                {
                    if (ImGui::IsMouseClicked(0))
                    {
                    //    showFull = !showFull;
                    }

                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(GetThreadString(v.id));
                    ImGui::SameLine();
                    ImGui::TextDisabled("(0x%" PRIx64 ")", v.id);

#if false
                    if (!v->timeline.empty())
                    {
                        ImGui::Separator();
                        TextFocused("Appeared at", TimeToString(v->timeline.front()->start - GetTimeBegin()));
                        TextFocused("Zone count:", RealToString(v->count, true));
                        TextFocused("Top-level zones:", RealToString(v->timeline.size(), true));
                    }
#endif
                    ImGui::EndTooltip();
                }
            }

            offset += ostep;

            if (showFull)
            {
#if 1
            //    m_lastCpu = -1;
            //    if (m_drawZones)
                {
                    const auto depth = DispatchZoneLevel(v.zones, hover, pxns, wpos, offset, 0, yMin, yMax);
                    offset += ostep * depth;
                }
#endif

            //    if (m_drawLocks)
            //    {
            //        const auto depth = DrawLocks(v->id, hover, pxns, wpos, offset, nextLockHighlight, yMin, yMax);
            //        offset += ostep * depth;
            //    }
            }
            offset += ostep * 0.2f;
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
            draw->AddRectFilled(ImVec2(wpos.x + (s - view.zvStart_) * pxns, linepos.y), ImVec2(wpos.x + (e - view.zvStart_) * pxns, linepos.y + lineh), 0x22DD8888);
            draw->AddRect(ImVec2(wpos.x + (s - view.zvStart_) * pxns, linepos.y), ImVec2(wpos.x + (e - view.zvStart_) * pxns, linepos.y + lineh), 0x44DD8888);

            ImGui::BeginTooltip();
            ImGui::TextUnformatted(TimeToString(e - s));
            ImGui::EndTooltip();
        }
        else if (drawMouseLine)
        {
            auto& io = ImGui::GetIO();
            draw->AddLine(ImVec2(io.MousePos.x, linepos.y), ImVec2(io.MousePos.x, linepos.y + lineh), 0x33FFFFFF);
        }

    }

    static void DrawFrame(Server& srv, float ww, float wh)
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


        if (show_app_metrics) { ImGui::ShowMetricsWindow(&show_app_metrics); }
        if (show_app_style_editor) { ImGui::Begin("Style Editor", &show_app_style_editor); ImGui::ShowStyleEditor(); ImGui::End(); }
        if (show_app_about) { ImGui::ShowAboutWindow(&show_app_about); }

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

    //    float h = wh - menuBarSize.y;
        const float defauktWorkspaceWidth = 300.f;
        const float minWorkspaceWidth = 128.f;
        const float minTracesWidth = 256.f;

        static float sz1 = defauktWorkspaceWidth;
        static float sz2 = ww - sz1;

        static float lastWidth = ww;

        if (ww != lastWidth) {

            // lets just build a new ratio
            float leftR = sz1 / lastWidth;
            float rightR = sz2 / lastWidth;

            sz1 = ww * leftR;
            sz2 = ww * rightR;

            lastWidth = ww;
        }

        Splitter(true, 4.0f, &sz1, &sz2, minWorkspaceWidth, minTracesWidth);
        {
            ImGui::BeginChild("1", ImVec2(sz1, -1), false);

            if (ImGui::BeginTabBar("Main Tabs"))
            {
                if (ImGui::BeginTabItem("Traces", nullptr, 0))
                {
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    // draw me a list like a pickle in the wind.
                    X_UNUSED(srv);
                    for (const auto& app : srv.apps)
                    {
                        if (ImGui::CollapsingHeader(app.appName.c_str()))
                        {
                            // ImGui::Text(app.appName.c_str());
                            // ImGui::Text("Num %" PRIuS, app.traces.size());
                            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.2f, 0.2f, 0.2f));

                            static int selected = -1;

                            for (int32_t i=0; i<static_cast<int32_t>(app.traces.size()); i++)
                            {
                                const auto& trace = app.traces[i];

                                if (ImGui::Selectable(trace.name.c_str(), selected == i))
                                {
                                    selected = i;
                                }
                            }

                            ImGui::PopStyleColor(1);
                        }
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
            ImGui::BeginChild("2", ImVec2(sz2, -1), false);

            if (ImGui::BeginTabBar("View Tabs"))
            {
                if (ImGui::BeginTabItem("Traces", nullptr, 0))
                {
                    DrawFrames();
                    DrawZones();

                    ImGui::EndTabItem();
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

    void sendDataToServer(Server& srv, const void* pData, int32_t len)
    {
#if X_DEBUG
        if (len > MAX_PACKET_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        // send some data...
        // TODO: none blocking?
        int res = platform::send(srv.socket, reinterpret_cast<const char*>(pData), len, 0);
        if (res == SOCKET_ERROR) {
            X_ERROR("Telem", "Socket: send failed with error: %d", platform::WSAGetLastError());
            return;
        }
    }

    bool handleConnectionResponse(Server& srv, tt_uint8* pData, tt_size len)
    {
        X_UNUSED(srv, len);

        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);
        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequestAccepted:
                // don't care about response currently.
                return true;
            case PacketType::ConnectionRequestRejected: {
                auto* pConRej = reinterpret_cast<const ConnectionRequestRejectedHdr*>(pData);
                auto* pStrData = reinterpret_cast<const char*>(pConRej + 1);
                X_ERROR("Telem", "Connection rejected: %.*s", pConRej->reasonLen, pStrData);
            }
            default:
                return false;
        }
    }

    bool readPacket(Server& srv, char* pBuffer, int& bufLengthInOut)
    {
        // this should return complete packets or error.
        int bytesRead = 0;
        int bufLength = sizeof(PacketBase);

        while (1) {
            int maxReadSize = bufLength - bytesRead;
            int res = platform::recv(srv.socket, &pBuffer[bytesRead], maxReadSize, 0);

            if (res == 0) {
                X_ERROR("Telem", "Connection closing...");
                return false;
            }
            else if (res < 0) {
                X_ERROR("Telem", "recv failed with error: %d", platform::WSAGetLastError());
                return false;
            }

            bytesRead += res;

            X_LOG0("Telem", "got: %d bytes\n", res);

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

    bool connectToServer(Server& srv)
    {
        const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

        struct platform::addrinfo hints, *servinfo = nullptr;
        core::zero_object(hints);
        hints.ai_family = AF_UNSPEC; // ipv4/6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = platform::IPPROTO_TCP;

        // Resolve the server address and port
        auto res = platform::getaddrinfo("127.0.0.1", "8001", &hints, &servinfo);
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

        srv.socket = connectSocket;
        sendDataToServer(srv, &cr, sizeof(cr));

        // wait for a response O.O
        char recvbuf[MAX_PACKET_SIZE];
        int recvbuflen = sizeof(recvbuf);

        // TODO: support timeout.
        if (!readPacket(srv, recvbuf, recvbuflen)) {
            return false;
        }

        if (!handleConnectionResponse(srv, reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(recvbuflen))) {
            return false;
        }

        return true;
    }



    void getAppList(Server& srv)
    {
        QueryApps qa;
        qa.dataSize = sizeof(qa);
        qa.type = PacketType::QueryApps;
        qa.offset = 0;
        qa.max = 64;

        sendDataToServer(srv, &qa, sizeof(qa));

        // get response.
        char recvbuf[MAX_PACKET_SIZE];
        int recvbuflen = sizeof(recvbuf);

        if (!readPacket(srv, recvbuf, recvbuflen)) {
            return;
        }

        auto* pHdr = reinterpret_cast<const QueryAppsResponseHdr*>(recvbuf);
        if (pHdr->type != PacketType::QueryAppsResp) {
            return;
        }

        auto* pApps = reinterpret_cast<const QueryAppsResponseData*>(pHdr + 1);

        for (int32_t i = 0; i < pHdr->num; i++)
        {
            telemetry::TraceApp app(g_arena);
            app.appName.set(pApps[i].appName);

            srv.apps.push_back(app);
        }

        for (auto& app : srv.apps)
        {
            QueryAppTraces qat;
            qat.dataSize = sizeof(qat);
            qat.type = PacketType::QueryAppTraces;

            sendDataToServer(srv, &qat, sizeof(qat));

            recvbuflen = sizeof(recvbuf);
            if (!readPacket(srv, recvbuf, recvbuflen)) {
                return;
            }

            auto* pTraceHdr = reinterpret_cast<const QueryAppTracesResponseHdr*>(recvbuf);
            if (pTraceHdr->type != PacketType::QueryAppTracesResp) {
                return;
            }

            auto* pTraces = reinterpret_cast<const QueryAppTracesResponseData*>(pHdr + 1);

            for (int32_t i = 0; i < pTraceHdr->num; i++)
            {
                telemetry::Trace trace;
                trace.name.assign(pTraces[i].name);
                trace.buildInfo.assign(pTraces[i].buildInfo);

                app.traces.push_back(std::move(trace));
            }
        }
    }

#endif

} // namespace



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(nCmdShow);

    TelemetryViewerArena::AllocationPolicy allocator;
    TelemetryViewerArena arena(&allocator, "TelemetryViewerArena");

    g_arena = &arena;

    {
        EngineApp app;

        if (!app.Init(hInstance, &arena, lpCmdLine)) {
            return 1;
        }

        if (!winSockInit()) {
            return 1;
        }

        // TEMP: connect to server on start up.
        // Server srv(g_arena);
        // connectToServer(srv);
        // getAppList(srv);
        {
            telemetry::Client client(&arena);

            if (!telemetry::run(client)) {
                return 1;
            }
        }

        winSockShutDown();
    }

    return 0;
}


