#pragma once

#ifndef _X_PROFILE_H_
#define _X_PROFILE_H_

#include <Containers\Array.h>

#include "Vars\ProfilerVars.h"

X_NAMESPACE_DECLARE(render, struct IRender);

X_NAMESPACE_DECLARE(engine,
	class IPrimativeContext;
);



X_NAMESPACE_BEGIN(core)

#if 1

namespace profiler
{

	class XProfileSys : 
		public IProfiler,
		public ICoreEventListener
	{
		struct SubSystemInfo
		{
			const char* pName;
			Colorf col;
		};

		typedef std::array<SubSystemInfo, profiler::SubSys::ENUM_COUNT> SubSystemInfoArr;
		typedef core::Array<XProfileData*>	ProfilerDataPtrArr;
		typedef core::Array<XProfileDataHistory*>	ProfilerDataHistoryPtrArr;


	public:
		XProfileSys(core::MemoryArenaBase* arena);
		~XProfileSys() X_FINAL;

		void registerVars(void);
		void registerCmds(void);

		bool init(ICore* pCore);
		bool loadRenderResources(void);
		void shutDown(void);

		// IProfiler
		void AddProfileData(XProfileData* pData) X_FINAL;

		void ScopeBegin(XProfileScope* pScope) X_FINAL;
		void ScopeEnd(XProfileScope* pScope) X_FINAL;
		// ~IProfiler

		void OnFrameBegin(void);
		void OnFrameEnd(void);

		void Render(void);

		X_INLINE const ProfilerVars& getVars(void) const;

	private:
		// ICoreEventListener		
		void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam);

	private:
		void UpdateProfileData(void);



	private:
		ProfilerVars vars_;

		SubSystemInfoArr subSystemInfo_;

		// rendering stuff
		Vec2i renderRes_;
		font::IFont* pFont_;

		ProfilerDataPtrArr profilerData_;
		ProfilerDataHistoryPtrArr profilerHistoryData_;

		uint64_t frameStartTime_;
		uint64_t frameTime_;
		uint64_t totalTime_;
	};

} // namespace profiler

#else

class XProfileSys : public IProfileSys
{

public:
	XProfileSys(core::MemoryArenaBase* arena);
	~XProfileSys() X_FINAL;

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(ICore* pCore) X_FINAL;
	void shutDown(void) X_FINAL;

	bool loadRenderResources(void) X_FINAL;

	void AddProfileData(XProfileData* pData) X_FINAL;

	void OnFrameBegin(void) X_FINAL;
	void OnFrameEnd(void) X_FINAL;

	void Render(void) X_FINAL;

private:
	void UpdateProfileData(void);

	static void ScopeStart(XProfileScope* pProScope);
	static void ScopeEnd(XProfileScope* pProScope);

	static XProfileSys* s_this;

private:

	class XStack
	{
	public:
		XStack() :
			pCurrent_(nullptr)
		{

		}

		X_INLINE void Push(XProfileScope* pScope)
		{
			pScope->pParent_ = pCurrent_;
			pCurrent_ = pScope;
		}

		X_INLINE void Pop(XProfileScope* pScope)
		{
			pCurrent_ = pScope->pParent_;
		}

	private:
		XProfileScope* pCurrent_;
	};

	XStack callstack_;

	struct ProfileDisplayInfo
	{
		int depth;
		XProfileData* pData;
	};

	struct XSubSystemInfo
	{
		const char* name;
		float selfTime;
		float avg;

		void calAvg(void) {
			selfhistory.append(selfTime);
			avg = selfhistory.getAvg();
		}

		ProfilerHistory<float, X_PROFILE_HISTORY_SIZE> selfhistory;
	};


	void AddProfileDisplayData_r(XProfileData* pData, int lvl);
	void DisplayProfileData(void);

public:

	X_INLINE bool isEnabled(void) const {
		return enabled_;
	}

	X_INLINE void setEnabled(bool val) {
		enabled_ = val;
	}

private:

	Vec2f RenderSubSysInfo(const Vec2f& pos, const float width);
	Vec2f RenderMemoryInfo(const Vec2f& pos, const float height);
	Vec2f RenderProfileData(const Vec2f& pos, const float width);
	Vec2f RenderProfileDataHeader(const Vec2f& pos, const float width);
	Vec2f RenderFrameTimes(const Vec2f& pos, const float width, const float height);


	void DrawLabel(float x, float y, const char* pStr, const Color& col);
	void DrawLabel(float x, float y, const char* pStr, const Color& col, font::DrawTextFlags flags);

	void DrawRect(float x1, float y1, float x2, float y2, const Color& col);
	void DrawRect(Vec4f& rec, const Color& col);


	void DrawPercentageBlock(float x, float y,
		float width, float height, float ms, float percent,
		const char* name);

	// ~Rendering
	void UpdateSubSystemInfo(void);
	void ClearSubSystems(void);

private:
	typedef core::Array<XProfileData*>	Profilers;
	typedef core::Array<ProfileDisplayInfo>	DisplayInfo;

	typedef core::ProfilerHistory<float, X_PROFILE_HISTORY_SIZE> FrameTimes;


	ICore*		pCore_;
	render::IRender* pRender_;
	engine::IPrimativeContext* pPrimCon_;
	font::IFont* pFont_;

	Profilers	profiles_;
	DisplayInfo displayInfo_;

	uint64_t	frameStartTime_;
	uint64_t	frameTime_;
	uint64_t	totalTime_;

	XSubSystemInfo subSystemInfo_[profiler::SubSys::ENUM_COUNT];
	float subSystemTotal_;

	bool enabled_;
	bool _pad[3];

	FrameTimes  frameTimeHistory_;

	// vars
	int s_drawProfileInfo_;
	int s_drawProfileInfoWhenConsoleExpaned_;
	int s_drawSubsystems_;
	int s_drawMemInfo_;
	int s_drawStats_;
	int s_drawFrameTimeBar_;
};

#endif

X_NAMESPACE_END

#include "XProfile.inl"

#endif // !_X_PROFILE_H_


