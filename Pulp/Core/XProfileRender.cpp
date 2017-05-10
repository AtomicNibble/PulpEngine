#include "stdafx.h"
#include "XProfile.h"

#include "Profile\ProfilerTypes.h"

#include <String\HumanSize.h>

#include <IRender.h>
#include <I3DEngine.h>
#include <IConsole.h>
#include <IPrimativeContext.h>

X_NAMESPACE_BEGIN(core)

using namespace render;

namespace
{
	static const float ROW_SIZE = 11;
	static const float COL_SIZE = 11;

}

#if 0

void XProfileSys::Render(void)
{
	if (!s_drawProfileInfo_) {
		return;
	}

	pRender_ = gEnv->pRender;
	pPrimCon_ = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PROFILE);
	if (!pRender_ || !pPrimCon_) {
		return;
	}

	pPrimCon_->reset();

	UpdateSubSystemInfo(); // calculate avg's

	consoleState::Enum consoleState = gEnv->pConsole->getVisState();
	if (consoleState == consoleState::EXPANDED) {
		if (!s_drawProfileInfoWhenConsoleExpaned_) {
			return;
		}
	}

	const float X_START = 5.f;
	const float X_PAD = 5.f;
	const float Y_PAD = 5.f;
	const float WIDTH = 790.f;

	float Y_START = 5.f;
	if (consoleState != consoleState::CLOSED) {
		Y_START = 35.f;
	}

	Vec2f xy(X_START, Y_START);

	{
		// each block can grow based on it's contents.
		
		// sub system + mem stats.
		{
			Vec2f size;

			if (s_drawSubsystems_) 
			{
				size = RenderSubSysInfo(xy, WIDTH);
			}

			if (s_drawMemInfo_)
			{
				Vec2f memPos(xy.x + size.x, xy.y);

				if (s_drawSubsystems_) {
					memPos.x += X_PAD;
				}

				Vec2f memSize = RenderMemoryInfo(memPos, size.y);
				size.y = core::Max(size.y, memSize.y);
			}

			xy.y += size.y;
			xy.y += Y_PAD;
		}
		// draw the table.
		if (s_drawStats_)
		{
			Vec2f size = RenderProfileData(xy, WIDTH);

			xy.y += size.y;
			xy.y += Y_PAD;
		}
		// frame times bar
		if (s_drawFrameTimeBar_)
		{

			RenderFrameTimes(xy, WIDTH, 20.f);
		}

	}

	ClearSubSystems(); // reset for next frame.

}

Vec2f XProfileSys::RenderSubSysInfo(const Vec2f& pos, const float maxWidth)
{
	X_UNUSED(maxWidth);

	uint32_t i;
	const float width = 200;
	const float item_padding = 5;
	const float item_height = 20;
	const float item_width = width - (item_padding * 2);
	const float title_height = 20;

	Vec2f sizeOut;

	sizeOut.x = width;
	sizeOut.y = (item_height + item_padding) * (ProfileSubSys::ENUM_COUNT - 1);
	sizeOut.y += item_padding; // bottom badding
	sizeOut.y += title_height;

	pPrimCon_->drawQuad(pos.x, pos.y, width, sizeOut.y, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	Vec3f txtPos(pos.x + (width / 2), pos.y, 1);
	font::TextDrawContext con;
	con.pFont = pFont_;
	con.SetDefaultEffect();
	con.col = Col_Red;
	con.flags = font::DrawTextFlag::CENTER;
	pPrimCon_->drawText(txtPos, con, "Sub Systems");

	float percent;
	float total;
	float item_x, item_y;

	percent = 0.f;
	total = subSystemTotal_;
	item_x = pos.x + item_padding;
	item_y = pos.y + item_padding + title_height;

	for (i = 0; i < ProfileSubSys::ENUM_COUNT - 1; i++)
	{
		const XSubSystemInfo& sub = subSystemInfo_[i];

		if (total > 0.f) {
			percent = (sub.avg / total);
		}

		DrawPercentageBlock(item_x, item_y, item_width, item_height, sub.selfTime, percent, sub.name);
		item_y += (item_height + item_padding);
	}

	return sizeOut;
}

Vec2f XProfileSys::RenderMemoryInfo(const Vec2f& pos, float height)
{
	core::MemoryArenaBase* arena = g_coreArena;// gEnv->pArena;
	core::MemoryAllocatorStatistics allocStats = arena->getAllocatorStatistics(true);
	core::StackString512 str;
	core::HumanSize::Str temp;

	str.appendFmt("Num:%" PRIuS "\n", allocStats.allocationCount_);
	str.appendFmt("Num(Max):%" PRIuS "\n", allocStats.allocationCountMax_);
	str.appendFmt("Physical:%s\n", core::HumanSize::toString(temp, allocStats.physicalMemoryAllocated_));
	str.appendFmt("Physical(Used):%s\n", core::HumanSize::toString(temp, allocStats.physicalMemoryUsed_));
	str.appendFmt("Virtual(Res):%s\n", core::HumanSize::toString(temp, allocStats.virtualMemoryReserved_));
	str.appendFmt("WasteAlign:%s\n", core::HumanSize::toString(temp, allocStats.wasteAlignment_));
	str.appendFmt("WasteUnused:%s\n", core::HumanSize::toString(temp, allocStats.wasteUnused_));
	str.appendFmt("Overhead:%s\n", core::HumanSize::toString(temp, allocStats.internalOverhead_));


	Color txt_col(0.7f, 0.7f, 0.7f, 1.f);

	const float width = 200.f;
	if (height < 1.f) {
		height = 200.f;
	}

	pPrimCon_->drawQuad(pos.x, pos.y, width, height, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	DrawLabel(pos.x + (width / 2), pos.y, "Combined Mem Stats(Sys)", Col_Red,
		font::DrawTextFlag::CENTER);
	DrawLabel(pos.x + 5, pos.y + 20, str.c_str(), txt_col);

	// string stats.
	Vec2f stringPos(pos);
	stringPos.x += 5;
	stringPos.x += width;

	allocStats = gEnv->pStrArena->getAllocatorStatistics();

	str.clear();
	str.appendFmt("Num:%i\n", allocStats.allocationCount_);
	str.appendFmt("Num(Max):%i\n", allocStats.allocationCountMax_);
	str.appendFmt("Physical:%s\n", core::HumanSize::toString(temp, allocStats.physicalMemoryAllocated_));
	str.appendFmt("Physical(Used):%s\n", core::HumanSize::toString(temp, allocStats.physicalMemoryUsed_));
	str.appendFmt("Virtual(Res):%s\n", core::HumanSize::toString(temp, allocStats.virtualMemoryReserved_));
	str.appendFmt("WasteAlign:%s\n", core::HumanSize::toString(temp, allocStats.wasteAlignment_));
	str.appendFmt("WasteUnused:%s\n", core::HumanSize::toString(temp, allocStats.wasteUnused_));
	str.appendFmt("Overhead:%s\n", core::HumanSize::toString(temp, allocStats.internalOverhead_));

	pPrimCon_->drawQuad(stringPos.x, stringPos.y, width, height, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	DrawLabel(stringPos.x + (width / 2), stringPos.y, "String Memory Stats(Sys)", Col_Red,
		font::DrawTextFlag::CENTER);

	DrawLabel(stringPos.x + 5, stringPos.y + 20, str.c_str(), txt_col);

	stringPos.x += width;

	return Vec2f(stringPos.x - pos.x, height);
}

Vec2f XProfileSys::RenderProfileData(const Vec2f& pos, const float width)
{
	Vec2f hdrSize = RenderProfileDataHeader(pos, width);

	const float row_height = 12;
	const float depth_width = 16;

	const size_t numItems = displayInfo_.size();
	const float height = (numItems * row_height) + 5; // 5 pad

	const float line_height = height - 1;
	const float x = pos.x;
	const float y = pos.y + hdrSize.y;

	Vec3f points[6] = {
		Vec3f(x + (width * 0.7f), y, 1),
		Vec3f(x + (width * 0.7f), y + line_height, 1),
		Vec3f(x + (width * 0.8f), y, 1),
		Vec3f(x + (width * 0.8f), y + line_height, 1),
		Vec3f(x + (width * 0.9f), y, 1),
		Vec3f(x + (width * 0.9f), y + line_height, 1),
	};

	// draw the block.
	pPrimCon_->drawQuad(x, y, width, height, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));
	pPrimCon_->drawLines(points, 6, Color(0.2f, 0.2f, 0.2f, 1.f));


	float x_pos = x + 5;
	float y_pos = y;

	core::StackString<32> total, self, calls;
	Color txt_col(0.7f, 0.7f, 0.7f, 1.f);
	size_t i;

	for (i = 0; i < numItems; i++)
	{
		const XProfileData* pData = displayInfo_[i].pData;
		const int depth = displayInfo_[i].depth;

		const float selfAvg = pData->selfTimeHistory_.getAvg();
		const float totalAvg = pData->totalTimeHistory_.getAvg();
		const int callCountAvg = pData->callCountHistory_.getAvg();

		total.appendFmt("%4.2f", totalAvg);
		self.appendFmt("%4.2f", selfAvg);
		calls.appendFmt("%i", callCountAvg);

		DrawLabel(x_pos + (depth_width * depth), y_pos, pData->nickName_, txt_col);
		//	DrawLabel(x_pos + (depth_width * depth), y_pos, pData->functionName_, txt_col);
		DrawLabel(x_pos + (width * 0.7f), y_pos, total.c_str(), txt_col);
		DrawLabel(x_pos + (width * 0.80f), y_pos, self.c_str(), txt_col);
		DrawLabel(x_pos + (width * 0.90f), y_pos, calls.c_str(), txt_col);

		total.clear();
		self.clear();
		calls.clear();

		y_pos += row_height;
	}

	return Vec2f(width, height) + hdrSize;
}


Vec2f XProfileSys::RenderProfileDataHeader(const Vec2f& pos, const float width)
{
	const float header_height = 20;
	const float header_padding = 0;

	pPrimCon_->drawQuad(pos.x, pos.y, width, header_height, Color(0.2f, 0.2f, 0.2f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	// draw the text slut.
	float offset = 3;

	DrawLabel(pos.x + offset, pos.y + (header_height / 2.f),
		"Name", Col_Whitesmoke, font::DrawTextFlag::CENTER_VER);

	offset += (width * 0.7f);
	DrawLabel(pos.x + offset, pos.y + (header_height / 2.f), 
		"Total", Col_Whitesmoke, font::DrawTextFlag::CENTER_VER);

	offset += (width * 0.10f);
	DrawLabel(pos.x + offset, pos.y + (header_height / 2.f), 
		"Self", Col_Whitesmoke, font::DrawTextFlag::CENTER_VER);

	offset += (width * 0.10f);
	DrawLabel(pos.x + offset, pos.y + (header_height / 2.f),
		"Calls", Col_Whitesmoke, font::DrawTextFlag::CENTER_VER);


	return Vec2f(width, header_height + header_padding);
}

Vec2f XProfileSys::RenderFrameTimes(const Vec2f& pos, const float width, const float height)
{
	const float bar_padding = 2;
	float heights[X_PROFILE_FRAME_TIME_HISTORY_SIZE];
	Rectf rect(pos.x, pos.y, pos.x + width, pos.y + height);

	uint32_t i, num = 0;

	pPrimCon_->drawQuad(pos.x, pos.y, rect.getWidth(), rect.getHeight(), Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.1f, 0.01f, 0.01f, 0.8f));

	FrameTimes::reverse_iterator it = frameTimeHistory_.rbegin();
	for (; it != frameTimeHistory_.rend(); ++it)
	{
		const float val = *it;
		heights[num++] = val;
	}

	float max = frameTimeHistory_.getMax();

	// make max 30ms if it's less.
	max = core::Max(50.f, max);

	for (i = 0; i < num; i++)
	{
		float original = heights[i];
		if (original > 0.f)
		{
			heights[i] = original / max;

			// make sure they don't get too small.
			if (heights[i] < 0.03f) {
				heights[i] = 0.03f;
			}
		}
	}

	pPrimCon_->drawBarChart(rect, num, heights, bar_padding, X_PROFILE_FRAME_TIME_HISTORY_SIZE, Col_Coral);

	return Vec2f(rect.getWidth(), rect.getHeight());
}



void XProfileSys::DrawLabel(float x, float y, const char* pStr, const Color& col)
{
	Vec3f pos(x, y, 1);
	font::TextDrawContext ctx;
	ctx.pFont = pFont_;
	ctx.SetDefaultEffect();
	ctx.col = col;
	pPrimCon_->drawText(pos, ctx, pStr);
}

void XProfileSys::DrawLabel(float x, float y, const char* pStr, const Color& col, font::DrawTextFlags flags)
{
	Vec3f pos(x,y, 1);
	font::TextDrawContext ctx;
	ctx.pFont = pFont_;
	ctx.SetDefaultEffect();
	ctx.col = col;
	ctx.flags |= flags;
	pPrimCon_->drawText(pos, ctx, pStr);
}


void XProfileSys::DrawRect(float x1, float y1, float x2, float y2, const Color& col)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(x1);
	X_UNUSED(y1);
	X_UNUSED(x2);
	X_UNUSED(y2);
	X_UNUSED(col);
}

void XProfileSys::DrawRect(Vec4f& rec, const Color& col)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(rec);
	X_UNUSED(col);
}

void XProfileSys::DrawPercentageBlock(float x, float y,
	float width, float height, float ms, float percent,
	const char* name)
{
	Color back = Color(0.01f, 0.01f, 0.01f, 0.6f);
	Color fill = Col_Green;

	core::StackString<64> percentStr;
	percentStr.appendFmt("%s %.2fms (%.1f%%)", name, ms, percent * 100);


	pPrimCon_->drawQuad(x, y, width, height, back);
	pPrimCon_->drawQuad(x, y, width * percent, height, fill);
	// 2.5% offset
	DrawLabel(x + (width / 40.f), y + (height / 2.f), percentStr.c_str(), Col_Whitesmoke,
		font::DrawTextFlag::CENTER_VER);
}


void XProfileSys::UpdateSubSystemInfo(void)
{
	uint32_t i;
	subSystemTotal_ = 0;
	for (i = 0; i < ProfileSubSys::ENUM_COUNT - 1; i++)
	{
		subSystemInfo_[i].calAvg();
		subSystemTotal_ += subSystemInfo_[i].avg;
	}
}

void XProfileSys::ClearSubSystems(void)
{
	// reset
	uint32_t i;
	for (i = 0; i < ProfileSubSys::ENUM_COUNT; i++) {
		subSystemInfo_[i].selfTime = 0.f;
	}
}

#endif

X_NAMESPACE_END