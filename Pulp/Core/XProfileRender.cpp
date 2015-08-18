#include "stdafx.h"
#include "XProfile.h"

#include "Profile\ProfilerTypes.h"

#include <String\HumanSize.h>

#include <IRender.h>

X_NAMESPACE_BEGIN(core)

using namespace render;

namespace
{
	static const float ROW_SIZE = 11;
	static const float COL_SIZE = 11;

}


void XProfileSys::Render(void)
{
	pRender_ = gEnv->pRender;
	if (!pRender_)
		return;

	float x, y;
	float width;
	float height;
	const float padding = 5;

	x = 5;
	y = 35;

	pRender_->Set2D(true);	
		pRender_->DrawQuad(x,y,790, 560, Color(0.1f,0.1f,0.1f,0.6f),
			Color(0.01f, 0.01f, 0.01f, 1.0f));

		x += padding;
		y += padding;

		UpdateSubSystemInfo(); // calculate avg's
		RenderSubSysInfo(x, y, width, height);

		RenderMemoryInfo(x + width, y, 790 - width, height);

	//	x += padding;
		y += height + padding;

		float full_width = 790 - (padding * 2);

		// send what's left.
		RenderProfileData(x, y, full_width, 560 - y);
		RenderFrameTimes(x, y + (565 - y) + padding, full_width, 20);

		ClearSubSystems(); // reset for next frame.
	
	pRender_->Set2D(false);

	// so console is on top.
	pRender_->FlushTextBuffer();
}

void XProfileSys::UpdateSubSystemInfo(void)
{
	uint32_t i;
	subSystemTotal_ = 0;
	for (i = 0; i < ProfileSubSys::ENUM_COUNT; i++)
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

void XProfileSys::RenderSubSysInfo(float x, float y, float& width_out, float& height_out)
{
	uint32_t i;
	const float width = 200;
	const float item_padding = 5;
	const float item_height = 20;
	const float item_width = width - (item_padding * 2);
	const float title_height = 20;

	width_out = width;
	height_out = (item_height + item_padding) * ProfileSubSys::ENUM_COUNT;
	height_out += item_padding; // bottom badding
	height_out += title_height;

	pRender_->DrawQuad(x, y, width, height_out, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	Vec3f pos(x + (width / 2), y, 1);
	XDrawTextInfo ti;
	ti.col = Col_Red;
	ti.flags = DrawTextFlags::POS_2D | DrawTextFlags::MONOSPACE | DrawTextFlags::CENTER;
	pRender_->DrawTextQueued(pos, ti, "Sub Systems");

	float percent;
	float total;
	float item_x, item_y;

	percent = 0;
	total = subSystemTotal_;
	item_x = x + item_padding;
	item_y = y + item_padding + title_height;

	for (i = 0; i < ProfileSubSys::ENUM_COUNT; i++)
	{
		const XSubSystemInfo& sub = subSystemInfo_[i];

		if (total > 0)
			percent = (sub.avg / total);

		DrawPercentageBlock(item_x, item_y, item_width, item_height, sub.selfTime, percent, sub.name);
		item_y += (item_height + item_padding);
	}
}


void XProfileSys::DrawPercentageBlock(float x, float y, 
	float width, float height, float ms, float percent,
	const char* name)
{
	Color back = Color(0.01f,0.01f,0.01f,0.6f);
	Color fill = Col_Green;

	core::StackString<64> percentStr;
	percentStr.appendFmt("%s %.2fms (%.1f%%)", name, ms, percent * 100);


	pRender_->DrawQuad(x,y, width, height, back);
	pRender_->DrawQuad(x, y, width * percent, height, fill);
	// 2.5% offset
	DrawLabel(x + (width/40.f), y + (height/2.f), percentStr.c_str(), Col_Whitesmoke,
		 DrawTextFlags::CENTER_VER);
}



void XProfileSys::RenderMemoryInfo(float x, float y, float width, float height)
{
	core::MemoryArenaBase* arena = g_coreArena;// gEnv->pArena;
	core::MemoryAllocatorStatistics allocStats = arena->getAllocatorStatistics(true);
	core::StackString512 str;
	core::HumanSize::Str temp;

	str.appendFmt("Num:%i\n", allocStats.m_allocationCount);
	str.appendFmt("Num(Max):%i\n", allocStats.m_allocationCountMax);
	str.appendFmt("Physical:%s\n", core::HumanSize::toString(temp, allocStats.m_physicalMemoryAllocated));
	str.appendFmt("Physical(Used):%s\n", core::HumanSize::toString(temp, allocStats.m_physicalMemoryUsed));
	str.appendFmt("Virtual(Res):%s\n", core::HumanSize::toString(temp, allocStats.m_virtualMemoryReserved));
	str.appendFmt("WasteAlign:%s\n", core::HumanSize::toString(temp, allocStats.m_wasteAlignment));
	str.appendFmt("WasteUnused:%s\n", core::HumanSize::toString(temp, allocStats.m_wasteUnused));
	str.appendFmt("Overhead:%s\n", core::HumanSize::toString(temp, allocStats.m_internalOverhead));


	Color txt_col(0.7f, 0.7f, 0.7f, 1.f);

	x += 5;
	width = 200;

	pRender_->DrawQuad(x, y, width, height, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	DrawLabel(x + (width / 2), y, "Combined Mem Stats(Sys)", Col_Red,
		DrawTextFlags::CENTER);
	DrawLabel(x + 5, y + 20, str.c_str(), txt_col);

	// string stats.
	x += 5;
	x += width;

	allocStats = gEnv->pStrArena->getAllocatorStatistics();

	str.clear();
	str.appendFmt("Num:%i\n", allocStats.m_allocationCount);
	str.appendFmt("Num(Max):%i\n", allocStats.m_allocationCountMax);
	str.appendFmt("Physical:%s\n", core::HumanSize::toString(temp, allocStats.m_physicalMemoryAllocated));
	str.appendFmt("Physical(Used):%s\n", core::HumanSize::toString(temp, allocStats.m_physicalMemoryUsed));
	str.appendFmt("Virtual(Res):%s\n", core::HumanSize::toString(temp, allocStats.m_virtualMemoryReserved));
	str.appendFmt("WasteAlign:%s\n", core::HumanSize::toString(temp, allocStats.m_wasteAlignment));
	str.appendFmt("WasteUnused:%s\n", core::HumanSize::toString(temp, allocStats.m_wasteUnused));
	str.appendFmt("Overhead:%s\n", core::HumanSize::toString(temp, allocStats.m_internalOverhead));

	pRender_->DrawQuad(x, y, width, height, Color(0.1f, 0.1f, 0.1f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	DrawLabel(x + (width / 2), y, "String Memory Stats(Sys)", Col_Red,
		DrawTextFlags::CENTER);

	DrawLabel(x + 5, y + 20, str.c_str(), txt_col);
}


void XProfileSys::RenderProfileData(float x, float y, float width, float height)
{
	size_t i;

	RenderProfileDataHeader(x,y,width,height);

	const float line_height = height - 1;
	Vec3f points[6] = {
		Vec3f(x + (width * 0.7f), y, 1),
		Vec3f(x + (width * 0.7f), y + line_height, 1),
		Vec3f(x + (width * 0.8f), y, 1),
		Vec3f(x + (width * 0.8f), y + line_height, 1),
		Vec3f(x + (width * 0.9f), y, 1),
		Vec3f(x + (width * 0.9f), y + line_height, 1),
	};

	// draw the block.
	pRender_->DrawQuad(x, y, width, height, Color(0.1f, 0.1f, 0.1f, 0.6f), 
		Color(0.01f, 0.01f, 0.01f, 0.8f));
	pRender_->DrawLines(points, 6, Color(0.2f,0.2f,0.2f,1.f));
	
	core::StackString<32> total, self, calls;
	float selfAvg, totalAvg;
	int callCountAvg;

	const float row_height = 12;
	const float depth_width = 16;
	float x_pos = x + 5;
	float y_pos = y;

	Color txt_col(0.7f,0.7f,0.7f,1.f);

	for (i = 0; i < displayInfo_.size(); i++)
	{		
		const XProfileData* pData = displayInfo_[i].pData;
		const int depth = displayInfo_[i].depth;
		
		selfAvg = pData->selfTimeHistory_.getAvg();
		totalAvg = pData->totalTimeHistory_.getAvg();
		callCountAvg = pData->callCountHistory_.getAvg();

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
}

void XProfileSys::RenderProfileDataHeader(float& x, float& y, float& width, float& height)
{
	const float header_height = 20;
	const float header_padding = 0;

	pRender_->DrawQuad(x, y, width, header_height, Color(0.2f, 0.2f, 0.2f, 0.6f),
		Color(0.01f, 0.01f, 0.01f, 0.8f));

	// draw the text slut.
	float offset = 3;

	DrawLabel(x + offset, y + (header_height / 2.f), "Name", Col_Whitesmoke, DrawTextFlags::CENTER_VER);
	
	offset += (width * 0.7f);
	DrawLabel(x + offset, y + (header_height / 2.f), "Total", Col_Whitesmoke, DrawTextFlags::CENTER_VER);

	offset += (width * 0.10f);
	DrawLabel(x + offset, y + (header_height / 2.f), "Self", Col_Whitesmoke, DrawTextFlags::CENTER_VER);

	offset += (width * 0.10f);
	DrawLabel(x + offset, y + (header_height / 2.f), "Calls", Col_Whitesmoke, DrawTextFlags::CENTER_VER);
	


	y += header_height + header_padding;
	height -= header_height + header_padding;
}


void XProfileSys::RenderFrameTimes(float x, float y, float width, float height)
{

//	pRender_->DrawQuad(x, y, width, height, Color(0.2f, 0.0f, 0.2f, 0.6f));
//	pRender_->DrawRect(x, y, width, height, Color(0.01f, 0.01f, 0.01f, 0.8f));

	// I wanna render a fucking block of chicken bars.
	const float bar_padding = 2;
	float heights[X_PROFILE_FRAME_TIME_HISTORY_SIZE];
	Rectf rect(x,y,x+width,y+height);

	// build some points baby.
	uint32_t i, num = 0;

	FrameTimes::reverse_iterator it = frameTimeHistory_.rbegin();
	for (; it != frameTimeHistory_.rend(); ++it)
	{
		const float val = *it;
		heights[num++] = val;
	}

	// need to make these point in the 0-1 range.
	// just have the highest point as 1?
	// might not look the best, but it sure is easy :P
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
			if (heights[i] < 0.03f)
				heights[i] = 0.03f;
		}
	}


	pRender_->DrawBarChart(rect, num, heights, bar_padding, X_PROFILE_FRAME_TIME_HISTORY_SIZE);
}




void XProfileSys::DrawLabel(float x, float y, const char* pStr, const Color& col)
{
	Vec3f pos(x, y, 1);
	XDrawTextInfo ti;
	ti.col = col;
	ti.flags = DrawTextFlags::POS_2D | DrawTextFlags::MONOSPACE;
	pRender_->DrawTextQueued(pos, ti, pStr);
}

void XProfileSys::DrawLabel(float x, float y, const char* pStr, const Color& col, Flags<DrawTextFlags> flags)
{
	Vec3f pos(x,y, 1);
	XDrawTextInfo ti;
	ti.col = col;
	ti.flags = DrawTextFlags::POS_2D | DrawTextFlags::MONOSPACE;
	ti.flags |= flags;
	pRender_->DrawTextQueued(pos, ti, pStr);
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

X_NAMESPACE_END