#pragma once

#ifndef _X_PROFILE_H_
#define _X_PROFILE_H_

#include <Containers\Array.h>

#include <IRender.h>
#include <IFont.h>

X_NAMESPACE_DECLARE(render, struct IRender);

X_NAMESPACE_DECLARE(engine,
	class IPrimativeContext;
);


#define X_PROFILE_FRAME_TIME_HISTORY_SIZE 64

X_NAMESPACE_BEGIN(core)

class XProfileSys : public IProfileSys
{

public:
	XProfileSys();
	~XProfileSys() X_FINAL;

	virtual void Init(ICore* pCore) X_FINAL;

	virtual void AddProfileData(XProfileData* pData) X_FINAL;

	virtual void OnFrameBegin(void) X_FINAL;
	virtual void OnFrameEnd(void) X_FINAL;

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
	// Rendering (called by core)
	void Render(void);

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

	typedef core::ProfilerHistory<float, X_PROFILE_FRAME_TIME_HISTORY_SIZE> FrameTimes;


	ICore*		pCore_;
	render::IRender* pRender_;
	engine::IPrimativeContext* pPrimCon_;

	Profilers	profiles_;
	DisplayInfo displayInfo_;

	uint64_t	frameStartTime_;
	uint64_t	frameTime_;
	uint64_t	totalTime_;

	XSubSystemInfo subSystemInfo_[ProfileSubSys::ENUM_COUNT];
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



X_NAMESPACE_END

#endif // !_X_PROFILE_H_


