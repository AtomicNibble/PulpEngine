#include "stdafx.h"

#if X_ENABLE_PROFILER

#include "XProfile.h"

#include <Time\StopWatch.h>
#include <Platform\Process.h>
#include <Threading\JobSystem2.h>

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <IFont.h>
#include <IConsole.h>
#include <IMaterial.h>

#include <../../tools/MaterialLib/MatLib.h>

#include "Profile\ProfilerTypes.h"

#include "FileSys/xFileSys.h"

#if X_COMPILER_CLANG
#include <cpuid.h>
#endif // !X_COMPILER_CLANG

X_NAMESPACE_BEGIN(core)

namespace
{
    X_INTRINSIC(__rdtsc)

    namespace ProfileTimer
    {
        static uint64_t g_cpuspeed = 1000000;

        X_INLINE uint64_t getTicks(void)
        {
            return __rdtsc();
        }

        X_INLINE uint64_t getTicksFlush(void)
        {
            int temp[4];

#if X_COMPILER_CLANG
            __cpuid(temp, temp[0], temp[1], temp[2], temp[3]);
#else
            __cpuid(temp, 0);
#endif // !X_COMPILER_CLANG

            return __rdtsc();
        }

        X_INLINE float32_t toMS(uint64_t t)
        {
            return float32_t(t) / g_cpuspeed;
        }

        void CalculateCPUSpeed()
        {
            uint64_t start, elapsed;
            const uint32_t delayMS = 1;

            core::Process pro = core::Process::getCurrent();

            // We want absolute maximum priority
            const auto priorityClass = pro.getPriorityClass();
            const auto curThreadPri = core::Thread::getPriority();

            pro.setPriorityClass(core::Process::Priority::REALTIME);
            core::Thread::setPriority(core::Thread::Priority::REALTIME);
            core::Thread::sleep(0); // Give up the rest of our timeslice so we don't get a context switch

            core::StopWatch time;

            auto overhead = time.GetMilliSeconds();
            time.Start();

            start = getTicksFlush();
            core::Thread::sleep(delayMS);
            elapsed = (getTicksFlush() - start);

            auto elapsedTime = time.GetMilliSeconds() - (overhead);

            // Reset priority and get speed
            core::Thread::setPriority(curThreadPri);
            pro.setPriorityClass(priorityClass);

            auto ticksPerMS = static_cast<uint64_t>(static_cast<double>(elapsed) / static_cast<double>(elapsedTime));

            g_cpuspeed = ticksPerMS;
            // g_cpuspeed = 3200000;
        }
        // 3200000000
        // 3184599450000000
        // 3193773880
        // 3198000000
        // 3198813020
        // 1000000000 - 1.0
        // 2800000000 - 2.8
        // 3200000000 - 3.2
        // 3200000000
    } // namespace ProfileTimer

}; // namespace

namespace profiler
{
    XProfileSys::XProfileSys(core::MemoryArenaBase* arena) :
        pFont_(nullptr),
        frameOffset_(0),
        frameStartTime_(0),
        frameTime_(0),
        totalTime_(0),
        profilerData_(arena),
        profilerHistoryData_(arena),
        eventListerReg_(false)

#if X_ENABLE_PROFILER_WARNINGS
        ,
        warningList_(arena)
#endif // !X_ENABLE_PROFILER_WARNINGS
    {
        repeatEventTimer_ = TimeVal(0ll);
        repeatEventInterval_ = TimeVal(0.05f);
        repeatEventInitialDelay_ = TimeVal(0.5f);

#if X_ENABLE_PROFILER_WARNINGS

        pWarnTotalMem_ = nullptr;
        pWarnTextureMem_ = nullptr;
        pWarnSndMem_ = nullptr;
        pWarnFileSys_ = nullptr;

#endif // !X_ENABLE_PROFILER_WARNINGS
    }

    XProfileSys::~XProfileSys()
    {
    }

    void XProfileSys::registerVars(void)
    {
        vars_.RegisterVars();
    }

    void XProfileSys::registerCmds(void)
    {
    }

    bool XProfileSys::init(ICore* pCore)
    {
        X_LOG0("ProfileSys", "Starting");

        eventListerReg_ = pCore->GetCoreEventDispatcher()->RegisterListener(this);

        ProfileTimer::CalculateCPUSpeed();

        static_assert(profiler::SubSys::ENUM_COUNT == 14, "Added subsys? this code needs updating");

        core::zero_object(subSystemInfo_);
        subSystemInfo_[profiler::SubSys::CORE].pName = "Core";
        subSystemInfo_[profiler::SubSys::ENGINE3D].pName = "3DEngine";
        subSystemInfo_[profiler::SubSys::FONT].pName = "Font";
        subSystemInfo_[profiler::SubSys::INPUT].pName = "Input";
        subSystemInfo_[profiler::SubSys::RENDER].pName = "Render";
        subSystemInfo_[profiler::SubSys::SCRIPT].pName = "Script";
        subSystemInfo_[profiler::SubSys::SOUND].pName = "Sound";
        subSystemInfo_[profiler::SubSys::GAME].pName = "Game";
        subSystemInfo_[profiler::SubSys::PHYSICS].pName = "Physics";
        subSystemInfo_[profiler::SubSys::NETWORK].pName = "Network";
        subSystemInfo_[profiler::SubSys::VIDEO].pName = "Video";
        subSystemInfo_[profiler::SubSys::UNCLASSIFIED].pName = "UnClassified";
        subSystemInfo_[profiler::SubSys::TOOL].pName = "Tool";
        subSystemInfo_[profiler::SubSys::UNITTEST].pName = "UnitTests";

        subSystemInfo_[profiler::SubSys::CORE].col = Col_Red;
        subSystemInfo_[profiler::SubSys::ENGINE3D].col = Col_Orange;
        subSystemInfo_[profiler::SubSys::FONT].col = Col_Yellow;
        subSystemInfo_[profiler::SubSys::INPUT].col = Col_Lime;
        subSystemInfo_[profiler::SubSys::RENDER].col = Col_Green;
        subSystemInfo_[profiler::SubSys::SCRIPT].col = Col_Cyan;
        subSystemInfo_[profiler::SubSys::SOUND].col = Color8u(67, 133, 255);
        subSystemInfo_[profiler::SubSys::GAME].col = Col_Purple;
        subSystemInfo_[profiler::SubSys::PHYSICS].col = Col_Magenta;
        subSystemInfo_[profiler::SubSys::NETWORK].col = Col_Pink;
        subSystemInfo_[profiler::SubSys::VIDEO].col = Col_Blueviolet;
        subSystemInfo_[profiler::SubSys::UNCLASSIFIED].col = Col_Coral;
        subSystemInfo_[profiler::SubSys::TOOL].col = Col_Teal;
        subSystemInfo_[profiler::SubSys::UNITTEST].col = Col_Olive;

        return true;
    }

    bool XProfileSys::loadRenderResources(void)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pFontSys);

        pFont_ = gEnv->pFontSys->getDefault();
        if (!pFont_) {
            return false;
        }

#if X_ENABLE_PROFILER_WARNINGS

        auto* pMatMan = gEnv->p3DEngine->getMaterialManager();

        subSystemInfo_[SubSys::PHYSICS].pWarningMat = pMatMan->loadMaterial("code/warnings/phys");
        subSystemInfo_[SubSys::NETWORK].pWarningMat = pMatMan->loadMaterial("code/warnings/net");
        subSystemInfo_[SubSys::SOUND].pWarningMat = pMatMan->loadMaterial("code/warnings/sound");
        subSystemInfo_[SubSys::INPUT].pWarningMat = pMatMan->loadMaterial("code/warnings/input");
        subSystemInfo_[SubSys::GAME].pWarningMat = pMatMan->loadMaterial("code/warnings/game");
        subSystemInfo_[SubSys::ENGINE3D].pWarningMat = pMatMan->loadMaterial("code/warnings/engine3d");

        pWarnTotalMem_ = pMatMan->loadMaterial("code/warnings/mem");
        pWarnTextureMem_ = pMatMan->loadMaterial("code/warnings/texture_mem");
        pWarnSndMem_ = pMatMan->loadMaterial("code/warnings/sound_mem");
        pWarnFileSys_ = pMatMan->loadMaterial("code/warnings/file");

#endif // !X_ENABLE_PROFILER_WARNINGS

        return true;
    }

    void XProfileSys::shutDown(void)
    {
        X_LOG0("ProfileSys", "Shutting Down");

        // ...
        auto* pCore = gEnv->pCore;

        if (eventListerReg_) {
            pCore->GetCoreEventDispatcher()->RemoveListener(this);
        }
    }

    bool XProfileSys::asyncInitFinalize(void)
    {
#if X_ENABLE_PROFILER_WARNINGS

        if (!gEnv->p3DEngine) {
            return false;
        }

        auto* pMatMan = gEnv->p3DEngine->getMaterialManager();

        for (auto& sub : subSystemInfo_) {
            if (sub.pWarningMat) {
                sub.pWarningMat->waitForLoad(pMatMan);
            }
        }

        pWarnTotalMem_->waitForLoad(pMatMan);
        pWarnTotalMem_->waitForLoad(pMatMan);
        pWarnTextureMem_->waitForLoad(pMatMan);
        pWarnSndMem_->waitForLoad(pMatMan);
        pWarnFileSys_->waitForLoad(pMatMan);

#endif // !X_ENABLE_PROFILER_WARNINGS

        return true;
    }

    void XProfileSys::AddProfileData(XProfileData* pData)
    {
        if (pData->getType() == XProfileData::Type::SingleShot) {
            X_ASSERT(profilerData_.find(pData) == decltype(profilerData_)::invalid_index, "Data node already added")();
            profilerData_.emplace_back(pData);
        }
        else {
            auto* pDataHistory = static_cast<decltype(profilerHistoryData_)::Type>(pData);
            X_ASSERT(profilerHistoryData_.find(pDataHistory) == decltype(profilerHistoryData_)::invalid_index, "Data node already added")();
            profilerHistoryData_.emplace_back(pDataHistory);
        }
    }

    void XProfileSys::ScopeBegin(XProfileScope* pScope)
    {
        pScope->start_ = ProfileTimer::getTicks();
        pScope->excludeTime_ = 0llu;
    }

    void XProfileSys::ScopeEnd(XProfileScope* pScope)
    {
        XProfileData* data = pScope->pData_;

        uint64_t end = ProfileTimer::getTicks();
        uint64_t totalTime = (end - pScope->start_);

        uint64_t selftime = totalTime - pScope->excludeTime_;

        data->time_ += totalTime;
        data->timeSelf_ += selftime;
        data->callCount_++;

        // in order to support callstacks i would need to make thread specific stacks.
        // otherwise shit will get messy.
        // for now i just won't bother with stacks.

#if 0
		if (pScope->pParent_)
		{
			XProfileScope* pScopeParent = pScope->pParent_;

			// If we have parent, add this counter total time to parent exclude time.
			pScopeParent->excludeTime_ += totalTime;
			if (!data->pParent_ && pScopeParent->pData_)
			{
				pScopeParent->pData_->hasChildren_ = 1;
				data->pParent_ = pScopeParent->pData_;
			}
		}
#endif
    }

    void XProfileSys::OnFrameBegin(const FrameTimeData& frameTimeInfo)
    {
        if (repeatEvent_.keyId != input::KeyId::UNKNOWN) {
            repeatEventTimer_ -= frameTimeInfo.unscaledDeltas[ITimer::Timer::UI];

            if (repeatEventTimer_.GetValue() < 0) {
                onInputEvent(repeatEvent_);
                repeatEventTimer_ = repeatEventInterval_;
            }
        }

        if (vars_.isPaused()) {
            return;
        }

        // ...
        frameStartTime_ = ProfileTimer::getTicks();
    }

    void XProfileSys::OnFrameEnd(void)
    {
        if (vars_.isPaused()) {
            return;
        }

        uint64_t end = ProfileTimer::getTicks();

        frameTime_ = end - frameStartTime_;
        totalTime_ += frameTime_;
        frameTimeHistory_.append(frameTime_);

        // update some time stats.
        UpdateProfileData();
    }

    // ICoreEventListener
    void XProfileSys::OnCoreEvent(const CoreEventData& ed)
    {
        if (ed.event == CoreEvent::RENDER_RES_CHANGED) {
            renderRes_.x = static_cast<int32_t>(ed.renderRes.width);
            renderRes_.y = static_cast<int32_t>(ed.renderRes.height);
        }
    }

    bool XProfileSys::onInputEvent(const input::InputEvent& event)
    {
        if (!vars_.getProlfilerDrawFlags()) {
            return false;
        }

#if !X_ENABLE_JOBSYS_PROFILER
        X_UNUSED(event);

        return false;
#else
        if (event.action == input::InputState::CHAR) {
            return false;
        }

        if (event.action == input::InputState::RELEASED) {
            repeatEvent_.keyId = input::KeyId::UNKNOWN;
        }

        if (event.action != input::InputState::PRESSED) {
            return false;
        }

        if (event.keyId != input::KeyId::LEFT_ARROW && event.keyId != input::KeyId::RIGHT_ARROW) {
            return false;
        }

        repeatEvent_ = event;
        repeatEventTimer_ = repeatEventInitialDelay_;

        switch (event.keyId) {
            case input::KeyId::LEFT_ARROW:
                frameOffset_ = (frameOffset_ + 1) & core::V2::JOBSYS_HISTORY_MASK;
                break;
            case input::KeyId::RIGHT_ARROW:
                if (frameOffset_ == 0) {
                    frameOffset_ = (core::V2::JOBSYS_HISTORY_COUNT - 1);
                }
                else {
                    --frameOffset_;
                }
                break;
            default:
                break;
        }

        return true;
#endif // !X_ENABLE_JOBSYS_PROFILER
    }

    void XProfileSys::UpdateProfileData(void)
    {
        for (auto* pData : profilerHistoryData_) {
            pData->onFrameBegin();
        }
    }

    Vec2f XProfileSys::Render(engine::IPrimativeContext* pPrim, Vec2f pos, 
        const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys)
    {
        int32_t drawFlags = vars_.getProlfilerDrawFlags();
        if (!vars_.drawProfilerConsoleExpanded()) {
            if (gEnv->pConsole) {
                if (gEnv->pConsole->getVisState() == core::consoleState::EXPANDED) {
                    drawFlags = 0;
                }
            }
        }

        const float padding = 10;

        if (!drawFlags) {
            return pos;
        }

        Vec2f area;

        if (pJobSys && core::bitUtil::IsBitFlagSet(drawFlags, core::bitUtil::AlphaBit('j'))) {
#if X_ENABLE_JOBSYS_PROFILER
            int32_t profilerIdx = pJobSys->getCurrentProfilerIdx();

            // 0-15
            // we want one before
            int32_t historyOffset = frameOffset_ + 1;
            if (historyOffset > profilerIdx) {
                int32_t backNum = (historyOffset - profilerIdx);
                profilerIdx = core::V2::JOBSYS_HISTORY_COUNT - backNum;
            }
            else {
                profilerIdx -= historyOffset;
            }

            area = RenderJobSystem(pPrim, pos, frameTimeInfo, pJobSys, profilerIdx);
            pos.y += area.y + padding;
#else
            X_UNUSED(frameTimeInfo);
#endif // !X_ENABLE_JOBSYS_PROFILER
        }

        if (core::bitUtil::IsBitFlagSet(drawFlags, core::bitUtil::AlphaBit('s'))) {
            area = RenderStartupData(pPrim, pos);
            pos.x += area.x + padding;
        }

        if (core::bitUtil::IsBitFlagSet(drawFlags, core::bitUtil::AlphaBit('m'))) {
            auto strAllocStats = gEnv->pStrArena->getAllocatorStatistics();
            auto allocStats = gEnv->pArena->getAllocatorStatistics(true);

            core::MemoryAllocatorStatistics::Str str;
            strAllocStats.toString(str);

            area = RenderStr(pPrim, pos, L"String Mem", str);
            pos.x += area.x + padding;

            allocStats.toString(str);

            area = RenderStr(pPrim, pos, L"Combined Mem", str);
            pos.x += area.x + padding;
        }

#if X_ENABLE_FILE_STATS

        if (core::bitUtil::IsBitFlagSet(drawFlags, core::bitUtil::AlphaBit('f'))) {
            core::xFileSys* pFileSys = static_cast<core::xFileSys*>(gEnv->pFileSys);

            core::XFileStats::Str str1, str2;
            pFileSys->getStats().toString(str1);
            pFileSys->getStatsAsync().toString(str2);
            core::IOQueueStats::Str str3;
            pFileSys->getIOQueueStats().toString(str3);

            area = RenderStr(pPrim, pos, L"IO Stats", str1);
            pos.x += area.x + padding;

            area = RenderStr(pPrim, pos, L"IO Stats (Async)", str2);
            pos.x += area.x + padding;

            area = RenderStr(pPrim, pos, L"IO Qeue Stats", str3);
            pos.x += area.x + padding;
        }
#endif // !X_ENABLE_FILE_STATS

        if (core::bitUtil::IsBitFlagSet(drawFlags, core::bitUtil::AlphaBit('t'))) {
            area = RenderArenaTree(pPrim, pos, gEnv->pArena);
        }

        if (core::bitUtil::IsBitFlagSet(drawFlags, core::bitUtil::AlphaBit('r'))) {
            render::Stats::Str str;
            render::Stats stats = gEnv->pRender->getStats();

            stats.toString(str);

            area = RenderStr(pPrim, pos, L"Render Stats", str);
            pos.x += area.x + padding;
        }

#if X_ENABLE_PROFILER_WARNINGS
        drawWarnings(pPrim);
#endif // !X_ENABLE_PROFILER_WARNINGS
        
        return pos;
    }

    size_t RenderArenaTree_r(engine::IPrimativeContext* pPrim, font::TextDrawContext& ctx, Vec2f pos,
        int32_t treeIndent, core::MemoryArenaBase* arena)
    {
        auto arenaStats = arena->getStatistics();
        auto& allocStats = arenaStats.allocatorStatistics_;

        core::StackString<64, char> name;
        core::StackStringW256 str;
        core::HumanSize::Str strBuf, strBuf2;

        name.append(' ', treeIndent * 2);
        name.append(arenaStats.arenaName_);

        str.appendFmt(L"%-25" PRns "^6%6" PRIuS "%11" PRns "%11" PRns,
            name.c_str(),
            allocStats.allocationCount_,
            core::HumanSize::toString(strBuf, allocStats.physicalMemoryUsed_),
            core::HumanSize::toString(strBuf2, allocStats.physicalMemoryAllocated_));

        pPrim->drawText(pos.x, pos.y, ctx, str.begin(), str.end());

        size_t numChildren = 0;

        auto& children = arena->getChildrenAreas();
        for (size_t i = 0; i < children.size(); i++) {
            auto* pChildArena = children[i];

            ++numChildren;

            Vec2f p = pos;
            p.y += numChildren * 20.f;
            numChildren += RenderArenaTree_r(pPrim, ctx, p, treeIndent + 1, pChildArena);
        }

        return numChildren;
    }

    size_t XProfileSys::countChildren_r(core::MemoryArenaBase* arena)
    {
        auto& children = arena->getChildrenAreas();
        return core::accumulate(children.begin(), children.end(), 1_sz, [](core::MemoryArenaBase* arena) -> size_t {
            return countChildren_r(arena);
        });
    };

    Vec2f XProfileSys::RenderArenaTree(engine::IPrimativeContext* pPrim, Vec2f pos, core::MemoryArenaBase* arena)
    {
        const float padding = 10;
        //	const float treeIndent = 10.f;
        //	const float spacing = 20.f;

        const float colHdrStartX = pos.x;
        const float colHdrStartY = pos.y;
        const float colHdrHeight = 22.f;

        const float treeStartX = pos.x + padding;
        const float treeStartY = colHdrStartY + colHdrHeight;

        size_t numItems = countChildren_r(arena);

        const float width = 520;
        const float height = (20.f * numItems) + colHdrHeight;

        // background.
        pPrim->drawQuad(
            pos.x,
            pos.y,
            width,
            height,
            Color(0.1f, 0.1f, 0.1f, 0.8f));

        // draw background for coloum headers
        pPrim->drawQuad(
            colHdrStartX,
            colHdrStartY,
            width,
            20.f,
            Color(0.2f, 0.2f, 0.2f, 0.6f),
            Color(0.01f, 0.01f, 0.01f, 0.8f));

        // titles.
        core::StackStringW256 str;
        str.appendFmt(L"%-25" PRns "%6" PRns "%11" PRns "%11" PRns,
            "Memory Arena Name", "Num", "Phys(U)", "Phys");

        font::TextDrawContext ctx;
        ctx.pFont = pFont_;
        ctx.effectId = 0;
        ctx.SetColor(Col_White);
        ctx.SetSize(Vec2f(16.f, 16.f));
        pPrim->drawText(treeStartX, pos.y, ctx, str.begin(), str.end());

        ctx.SetColor(Col_Dimgray);
        RenderArenaTree_r(pPrim, ctx, Vec2f(treeStartX, treeStartY), 0, arena);

        return Vec2f(0.f, 0.f);
    }

    Vec2f XProfileSys::RenderStartupData(engine::IPrimativeContext* pPrim, Vec2f pos)
    {
        size_t maxNickNameWidth = 0;
        for (size_t i = 0; i < profilerData_.size(); ++i) {
            const auto* pData = profilerData_[i];
            maxNickNameWidth = core::Max(maxNickNameWidth, core::strUtil::strlen(pData->pNickName_));
        }

        // pad it
        maxNickNameWidth = core::Max(maxNickNameWidth + 2_sz, 5_sz);

        core::StackStringW512 text;
        text.setFmt(L"#%*s\t%5s\t%12s\t%8s", maxNickNameWidth, L"Name", L"Calls", L"Time(cs)", L"Time(ms)");

        font::TextDrawContext ctx;
        ctx.pFont = pFont_;
        ctx.effectId = 0;
        ctx.SetColor(Col_White);
        ctx.SetSize(Vec2f(16.f, 16.f));

        Vec2f titleSize = pFont_->GetTextSize(text.begin(), text.end(), ctx);

        const float padding = 10;

        //	const float titleStartX = pos.x + padding;
        const float titleStartY = pos.y;
        const float titleHeight = 16.f + padding;

        const float colHdrStartX = pos.x;
        const float colHdrStartY = titleStartY + titleHeight;
        const float colHdrHeight = 22.f;

        const float entryStartX = pos.x + padding;
        const float entryStartY = colHdrStartY + colHdrHeight;
        const float entryOffset = (ctx.size.y + 2.f);

        const float width = titleSize.x + (padding * 2);
        const float height = titleHeight + colHdrHeight + (profilerData_.size() * entryOffset) + (padding * 2);

        // background.
        pPrim->drawQuad(
            pos.x,
            pos.y,
            width,
            height,
            Color(0.1f, 0.1f, 0.1f, 0.8f));

        // draw background for coloum headers
        pPrim->drawQuad(
            colHdrStartX,
            colHdrStartY,
            width,
            20.f,
            Color(0.2f, 0.2f, 0.2f, 0.6f),
            Color(0.01f, 0.01f, 0.01f, 0.8f));

        // system cubes.
        std::sort(profilerData_.begin(), profilerData_.end(), [](const XProfileData* pLhs, const XProfileData* pRhs) {
            return pLhs->time_ > pRhs->time_;
        });

        for (size_t i = 0; i < profilerData_.size(); ++i) {
            const auto* pData = profilerData_[i];

            pPrim->drawQuad(
                entryStartX,
                entryStartY + (i * entryOffset),
                6.f, // intended as y
                ctx.size.y,
                subSystemInfo_[pData->subSystem_].col);
        }

        // headers
        ctx.SetColor(Col_Mintcream);
        ctx.flags.Set(font::DrawTextFlag::CENTER);

        pPrim->drawText(pos.x + (width / 2), titleStartY, ctx, L"Startup Timmings");

        ctx.SetColor(Col_Dimgray);
        ctx.flags.Remove(font::DrawTextFlag::CENTER);

        pPrim->drawText(entryStartX, colHdrStartY, ctx, text.begin(), text.end());

        ctx.SetColor(Col_Dimgray);

        for (size_t i = 0; i < profilerData_.size(); ++i) {
            const auto* pData = profilerData_[i];

            text.setFmt(L"%*" PRns "\t%5" PRIi32 "\t%12" PRIu64 "\t%8.2f",
                maxNickNameWidth + 1,
                pData->pNickName_,
                pData->callCount_,
                pData->time_,
                ProfileTimer::toMS(pData->time_));

            pPrim->drawText(entryStartX, entryStartY + (i * entryOffset), ctx, text.begin(), text.end());
        }

        return Vec2f(width, height);
    }

    Vec2f XProfileSys::RenderStr(engine::IPrimativeContext* pPrim, Vec2f pos, const wchar_t* pTitle, const core::StackString512& str)
    {
        font::TextDrawContext ctx;
        ctx.pFont = pFont_;
        ctx.effectId = 0;
        ctx.SetSize(Vec2f(16.f, 16.f));

        Vec2f size = pFont_->GetTextSize(str.begin(), str.end(), ctx);

        const float padding = 10;
        //	const float titleStartX = pos.x + padding;
        const float titleStartY = pos.y;
        const float titleHeight = 20.f;
        const float memInfoStartX = pos.x + padding;
        const float memInfoStartY = titleStartY + titleHeight;
        const float width = size.x + (padding * 2);
        const float height = titleHeight + size.y + (padding * 1);

        // okay so we have the stats should we just compute the full size?

        // background.
        pPrim->drawQuad(
            pos.x,
            pos.y,
            width,
            height,
            Color(0.1f, 0.1f, 0.1f, 0.8f));

        pPrim->drawQuad(
            pos.x,
            pos.y,
            width,
            20.f,
            Color(0.2f, 0.2f, 0.2f, 0.8f),
            Color(0.01f, 0.01f, 0.01f, 0.8f));

        ctx.SetColor(Col_Mintcream);
        ctx.flags.Set(font::DrawTextFlag::CENTER);
        pPrim->drawText(pos.x + (width / 2), titleStartY, ctx, pTitle);

        ctx.SetColor(Col_Dimgray);
        ctx.flags.Remove(font::DrawTextFlag::CENTER);
        pPrim->drawText(
            memInfoStartX,
            memInfoStartY,
            ctx,
            str.begin(), str.end());

        return Vec2f(width, height);
    }

#if X_ENABLE_JOBSYS_PROFILER

    Vec2f XProfileSys::RenderJobSystem(engine::IPrimativeContext* pPrim, Vec2f pos, 
        const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys, int32_t profileIdx)
    {
        X_UNUSED(pJobSys);
        X_UNUSED(profileIdx);
        auto& stats = pJobSys->GetStats();
        auto& frameStats = stats[profileIdx];
        auto& timeLines = pJobSys->GetTimeLines();

        const float padding = 10;
        const float border = 15;

        const float xStart = pos.x;
        // const float yStart = pos.y;
        const float width = renderRes_.x - (border * 2);
        const float maxHeight = (renderRes_.y - (border * 2) - pos.y);

        font::TextDrawContext ctx;
        ctx.pFont = pFont_;
        ctx.effectId = 0;
        ctx.SetColor(Col_White);
        ctx.SetSize(Vec2f(16.f, 16.f));

        float keyHeight = 30.f;
        const float maxHeightPerThread = 30.f;

        const int32_t visibleMS = vars_.jobSysThreadMS();
        //	const uint32_t numThread = pJobSys->GetThreadCount();
        const uint32_t numThreadQueues = pJobSys->GetQueueCount();
        const float threadInfoXOffset = 40.f;
        const float threadInfoX = xStart + padding + threadInfoXOffset;
        const float threadInfoY = pos.y + ctx.size.y + (padding * 2);
        const float threadInfoHeight = core::Min<float>((maxHeight - (ctx.size.y + padding) - (keyHeight + padding)) - 20, maxHeightPerThread * numThreadQueues);
        const float threadInfoWidth = (width - (padding * 2)) - threadInfoXOffset;

        const float widthPerMS = threadInfoWidth / visibleMS;
        const float threadInfoEntryHeight = threadInfoHeight / numThreadQueues;

        const float height = threadInfoHeight + (threadInfoY - pos.y) + (keyHeight * 2);

        std::array<int32_t, V2::JobSystem::HW_THREAD_MAX> jobCounts;

        for (uint32_t i = 0; i < numThreadQueues; i++) {
            if (!timeLines[i]) {
                continue;
            }

            jobCounts[i] = timeLines[i]->getHistory()[profileIdx].bottom_;
        }

        // draw a gird spliting up the horizontal space in to time.
        {
            // background
            pPrim->drawQuad(
                pos.x,
                pos.y,
                width,
                height,
                Color(0.1f, 0.1f, 0.1f, 0.8f));

            // draw a box showing what history index we on.
            {
                // should we only show offset instead of index?
                const float barWidth = 4.f;
                const float barSpacing = 2.f;
                const float barStartX = ((xStart + width) - (V2::JOBSYS_HISTORY_COUNT * (barWidth + barSpacing)) - padding);
                const float barStartY = pos.y + padding;

                int32_t invOffset = V2::JOBSYS_HISTORY_MASK - frameOffset_;

                for (int32_t i = 0; i < V2::JOBSYS_HISTORY_COUNT; i++) {
                    Color col = Color(0.3f, 0.3f, 0.3f);
                    if (i == invOffset) {
                        col = Col_Darkred;
                    }
                    else if (i == 0) {
                        col = Color(0.2f, 0.2f, 0.2f);
                    }

                    pPrim->drawQuad(
                        barStartX + (i * (barWidth + barSpacing)),
                        barStartY,
                        barWidth,
                        16,
                        col);
                }
            }

            for (int32_t i = 0; i < visibleMS + 1; i++) {
                Vec3f top(threadInfoX + (i * widthPerMS), threadInfoY, 1);
                Vec3f bottom(threadInfoX + (i * widthPerMS), threadInfoY + threadInfoHeight, 1);

                pPrim->drawLine(top, bottom, Colorf(0.2f, 0.2f, 0.2f));
            }

            X_DISABLE_WARNING(4127)

            ctx.col = Col_Dimgray;

            core::StackString<64> str;
            for (int32_t i = 1; i < visibleMS; i++) {
                if (visibleMS > 16) {
                    if (!(i & 1) || (i + 1) == visibleMS) {
                        continue;
                    }
                }

                str.setFmt("%ims", i);
                pPrim->drawText(threadInfoX + (i * widthPerMS), threadInfoY + threadInfoHeight, ctx, str.c_str());
            }

            // draw how many jobs each thread ran
            {
                ctx.flags.Set(font::DrawTextFlag::RIGHT);

                for (uint32_t i = 0; i < numThreadQueues; i++) {
                    str.setFmt("%i", jobCounts[i]);
                    pPrim->drawText(threadInfoX - padding, threadInfoY + (i * threadInfoEntryHeight), ctx, str.c_str());
                }

                ctx.flags.Remove(font::DrawTextFlag::RIGHT);
            }

            // draw the color key
            {
                float keyY = threadInfoY + (threadInfoHeight + keyHeight);
                float keyX = threadInfoX + padding;

                for (uint32_t i = 0; i < core::profiler::SubSys::ENUM_COUNT; i++) {
                    if (i == core::profiler::SubSys::UNITTEST) {
                        continue;
                    }

                    ctx.col = subSystemInfo_[i].col;

                    const char* pBegin = subSystemInfo_[i].pName;
                    const char* pEnd = pBegin + core::strUtil::strlen(subSystemInfo_[i].pName);

                    auto txtSize = pFont_->GetTextSize(pBegin, pEnd, ctx);

                    pPrim->drawText(keyX, keyY, ctx, subSystemInfo_[i].pName);

                    keyX += txtSize.x + 10.f;
                }
            }

            core::StackStringW512 txt;
            txt.appendFmt(L"JobsRun: %" PRIi32 " JobsStolen: %" PRIi32 " JobsAssited: %" PRIi32,
                int32_t(frameStats.jobsRun), int32_t(frameStats.jobsStolen), int32_t(frameStats.jobsAssited));

            ctx.SetColor(Col_Mintcream);
            ctx.flags.Set(font::DrawTextFlag::CENTER);
            pPrim->drawText(xStart + (width / 2), pos.y + padding, ctx, L"Profiler");

            ctx.flags.Remove(font::DrawTextFlag::CENTER);
            pPrim->drawText(xStart + padding, pos.y + padding, ctx, txt.begin(), txt.end());

            X_ENABLE_WARNING(4127)
        }

        SubSystemTimeArr subTimes;
        subTimes.fill(0);

        for (uint32_t i = 0; i < numThreadQueues; i++) {
            if (!timeLines[i]) {
                continue;
            }

            DrawThreadInfo(
                frameTimeInfo,
                pPrim,
                threadInfoX,
                threadInfoY + (i * threadInfoEntryHeight),
                threadInfoWidth,
                threadInfoEntryHeight - (padding * 1),
                timeLines[i]->getHistory()[profileIdx],
                subTimes);
        }

        DrawSubsysInfo(
            pPrim,
            threadInfoX,
            threadInfoY + threadInfoHeight + 22,
            threadInfoWidth,
            2,
            subTimes);

        return Vec2f(width, height);
    }

    void XProfileSys::DrawSubsysInfo(engine::IPrimativeContext* pPrim, float xStart, float yStart, float width, float height,
        const SubSystemTimeArr& subTimes)
    {
        // for a given width i want to split it up into subsystem percentages.
        auto total = core::TimeVal(core::accumulate(subTimes.begin(), subTimes.end(), 0ll));
        auto totalMS = total.GetMilliSeconds();
        const float visibleMS = totalMS;
        const float widthPerMS = width / visibleMS;

        float entryStart = 0;

        for (size_t i = 0; i < subTimes.size(); ++i) {
            auto time = core::TimeVal(subTimes[i]);
            auto timeMS = time.GetMilliSeconds();
            float entryWidth = timeMS * widthPerMS;

            auto col = subSystemInfo_[i].col;
            //	col.shade(-30.f);

            pPrim->drawQuad(xStart + entryStart, yStart, entryWidth, height, col);

            entryStart += timeMS * widthPerMS;
        }
    }

    void XProfileSys::DrawThreadInfo(const FrameTimeData& frameTimeInfo,
        engine::IPrimativeContext* pPrim, float xStart, float yStart, float width, float height,
        const core::V2::JobQueueHistory::FrameHistory& history, SubSystemTimeArr& subTimesOut)
    {
        pPrim->drawQuad(xStart, yStart, width, height, Color(0.05f, 0.05f, 0.05f, 0.1f));

        X_UNUSED(frameTimeInfo);
        X_UNUSED(history);

        if (history.bottom_ == 0) {
            return;
        }

        const int32_t visibleMS = vars_.jobSysThreadMS();
        const float widthPerMS = width / visibleMS;
        const float quadsize = height;

        // i want to draw the history of this thread at correct based on start time relative to
        // start time, so that I can visualize 'bubbles'.
        // this means i need to know when the frame started.
        const auto frameStartTime = history.start;

        const int32_t num = history.getMaxreadIdx();
        for (int32_t idx = 0; idx < num; idx++) {
            const auto& entry = history.entryes_[idx];
            const auto timeOffset = entry.start - frameStartTime;
            const auto elapsed = (entry.end - entry.start);

            subTimesOut[entry.subsystem] += elapsed.GetValue();

            float entryStart = timeOffset.GetMilliSeconds() * widthPerMS;
            float entryWidth = elapsed.GetMilliSeconds() * widthPerMS;
            float entryEnd = entryStart + entryWidth;

            if (entryEnd > width) {
                // make it clear it's overflowing time res.
                if (entryStart > width) {
                    entryStart = width;
                    entryWidth = 5.f;
                }
                else {
                    entryWidth = (width - entryStart) + 5.f;
                }
            }

            entryWidth = core::Max(entryWidth, 1.f);

            pPrim->drawQuad(xStart + entryStart, yStart, entryWidth, quadsize, subSystemInfo_[entry.subsystem].col);
        }
    }

#endif // !X_ENABLE_JOBSYS_PROFILER

#if X_ENABLE_PROFILER_WARNINGS

    void XProfileSys::drawWarnings(engine::IPrimativeContext* pPrim)
    {
        if (warningList_.isEmpty()) {
            return;
        }

        const core::TimeVal timeout(10.f);
        const auto timeNow = core::StopWatch::GetTimeNow();

        // remove any old warnings.
        for (auto it = warningList_.begin(); it != warningList_.end();) {
            if ((it->first + timeout) < timeNow) {
                it = warningList_.erase(it);
            }
            else {
                ++it;
            }
        }

        for (size_t i = 0; i < warningList_.size(); i++) {
            auto& warn = warningList_[i];
            pPrim->drawQuad(5.f, 40.f + (i * 35.f), 32.f, 32.f, warn.second, Col_White);
        }
    }

#endif // !X_ENABLE_PROFILER_WARNINGS

} // namespace profiler

X_NAMESPACE_END

#endif // !X_ENABLE_PROFILER