#include "stdafx.h"
#include "XGlyphBitmap.h"


X_NAMESPACE_BEGIN(font)

XGlyphBitmap::XGlyphBitmap() :
	pBuffer_(nullptr),
	width_(0),
	height_(0)
{

}

XGlyphBitmap::XGlyphBitmap(int32_t width, int32_t height) :
pBuffer_(nullptr),
width_(0),
height_(0)
{
	Create(width, height);
}



XGlyphBitmap::~XGlyphBitmap()
{
	Release();
}


bool XGlyphBitmap::Create(int32_t width, int32_t height)
{
	Release();

	pBuffer_ = X_NEW_ARRAY(uint8_t, width * height, g_fontArena,"BitMapBuffer");

	if (!pBuffer_) {
		return false;
	}

	width_ = width;
	height_ = height;
	return true;
}


void XGlyphBitmap::Release(void)
{
	if (pBuffer_) {
		X_DELETE_ARRAY(pBuffer_, g_fontArena);
		pBuffer_ = nullptr;
	}

	width_ = height_ = 0;
}

void XGlyphBitmap::Blur(int32_t iterations)
{
	int32_t cSum;
	int32_t yOffset;
	int32_t yupOffset;
	int32_t ydownOffset;
	int32_t i, x, y;

	for (i = 0; i < iterations; i++)
	{
		// loop over each row
		for (y = 0; y < height_; y++)
		{
			yOffset = y * width_;

			if (y - 1 >= 0)
			{
				yupOffset = (y - 1) * width_;
			}
			else
			{
				yupOffset = (y) * width_;
			}

			if (y + 1 < height_)
			{
				ydownOffset = (y + 1) * width_;
			}
			else
			{
				ydownOffset = (y) * width_;
			}

			// blur a row
			for (x = 0; x < width_; x++)
			{
				cSum = pBuffer_[yupOffset + x] + pBuffer_[ydownOffset + x];

				if (x - 1 >= 0)
				{
					cSum += pBuffer_[yOffset + x - 1];
				}
				else
				{
					cSum += pBuffer_[yOffset + x];
				}

				if (x + 1 < width_)
				{
					cSum += pBuffer_[yOffset + x + 1];
				}
				else
				{
					cSum += pBuffer_[yOffset + x];
				}

				pBuffer_[yOffset + x] = safe_static_cast<uint8_t, int32_t>(cSum >> 2);
			}
		}
	}
}

bool XGlyphBitmap::Scale(float scaleX, float scaleY)
{
	// anything to scale?
	if (!pBuffer_) {
		return false;
	}

	// Scale changed?
	Vec2f scale(scaleX, scaleY);
	Vec2f sameScale(1.f, 1.f);
	if (sameScale.compare(scale, EPSILON_VALUEf)) {
		return true;
	}

	// new buffer
	const int32_t newWidth =  static_cast<int32_t>(width_ * scaleX);
	const int32_t newHeight = static_cast<int32_t>(height_ * scaleY);

	uint8_t* pNewBuffer = X_NEW_ARRAY(uint8_t, newWidth * newHeight, g_fontArena, "ScaleBuffer");

	if (!pNewBuffer) {
		return false;
	}

	const float xFactor = width_ / static_cast<float>(newWidth);
	const float yFactor = height_ / static_cast<float>(newHeight);

	float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
	int32_t xCeil, yCeil, xFloor, yFloor, yNewOffset;

	uint8_t c0, c1, c2, c3;

	for (int32_t y = 0; y < newHeight; ++y)
	{
		yFractioned = y * yFactor;
		yFloor = static_cast<int32_t>(math<float>::floor(yFractioned));
		yCeil = yFloor + 1;

		if (yCeil >= height_) {
			yCeil = yFloor;
		}

		yFraction = yFractioned - yFloor;
		oneMinusY = 1.0f - yFraction;

		yNewOffset = y * newWidth;

		for (int32_t x = 0; x < newWidth; ++x)
		{
			xFractioned = x * xFactor;
			xFloor = static_cast<int32_t>(math<float>::floor(xFractioned));
			xCeil = xFloor + 1;

			if (xCeil >= width_) {
				xCeil = xFloor;
			}

			xFraction = xFractioned - xFloor;
			oneMinusX = 1.0f - xFraction;

			c0 = pBuffer_[yFloor * width_ + xFloor];
			c1 = pBuffer_[yFloor * width_ + xCeil];
			c2 = pBuffer_[yCeil * width_ + xFloor];
			c3 = pBuffer_[yCeil * width_ + xCeil];

			fR0 = (oneMinusX * c0 + xFraction * c1);
			fR1 = (oneMinusX * c2 + xFraction * c3);

			pNewBuffer[yNewOffset + x] = static_cast<uint8_t>((oneMinusY * fR0) + (yFraction * fR1));
		}
	}

	// update members
	width_ = newWidth;
	height_ = newHeight;

	X_DELETE_ARRAY(pBuffer_, g_fontArena);
	pBuffer_ = pNewBuffer;

	return true;
}

void XGlyphBitmap::Clear(void)
{
	if (pBuffer_) {
		memset(pBuffer_, 0, width_ * height_);
	}
}


//-------------------------------------------------------------------------------------------------
bool XGlyphBitmap::BlitScaledTo8(uint8_t* pBuffer, 
	int32_t srcX, int32_t srcY, 
	int32_t srcWidth, int32_t srcHeight, 
	int32_t destX, int32_t destY, 
	int32_t destWidth, int32_t destHeight, 
	int32_t destBufferWidth)
{
	X_UNUSED(destY);
	X_UNUSED(srcX);

	const int32_t newWidth =  static_cast<int32_t>(destWidth);
	const int32_t newHeight = static_cast<int32_t>(destHeight);

	uint8_t* pNewBuffer = pBuffer;

	const float xFactor = srcWidth / static_cast<float>(newWidth);
	const float yFactor = srcHeight / static_cast<float>(newHeight);

	float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
	int32_t xCeil, yCeil, xFloor, yFloor, yNewOffset;

	uint8_t c0, c1, c2, c3;

	for (int32_t y = 0; y < newHeight; ++y)
	{
		yFractioned = y * yFactor;
		yFloor = (int32_t)math<float>::floor(yFractioned);
		yCeil = yFloor + 1;

		yFraction = yFractioned - yFloor;
		oneMinusY = 1.0f - yFraction;

		yNewOffset = y * destBufferWidth;

		yFloor += srcY;
		yCeil += srcY;

		if (yCeil >= height_) {
			yCeil = yFloor;
		}

		for (int32_t x = 0; x < newWidth; ++x)
		{
			xFractioned = x * xFactor;
			xFloor = (int32_t)math<float>::floor(xFractioned);
			xCeil = xFloor + 1;

			xFraction = xFractioned - xFloor;
			oneMinusX = 1.0f - xFraction;

			xFloor += srcY;
			xCeil += srcY;

			if (xCeil >= width_) {
				xCeil = xFloor;
			}

			c0 = pBuffer_[yFloor * width_ + xFloor];
			c1 = pBuffer_[yFloor * width_ + xCeil];
			c2 = pBuffer_[yCeil * width_ + xFloor];
			c3 = pBuffer_[yCeil * width_ + xCeil];

			fR0 = (oneMinusX * c0 + xFraction * c1);
			fR1 = (oneMinusX * c2 + xFraction * c3);

			pNewBuffer[yNewOffset + x + destX] = (uint8_t)((oneMinusY * fR0) + (yFraction * fR1));
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------- 
bool XGlyphBitmap::BlitScaledTo32(unsigned char *pBuffer,
	int32_t srcX, int32_t srcY, 
	int32_t srcWidth, int32_t srcHeight, 
	int32_t destX, int32_t destY, 
	int32_t destWidth, int32_t destHeight, 
	int32_t destBufferWidth)
{
	X_UNUSED(destY);
	X_UNUSED(srcX);

	const int32_t newWidth = static_cast<int32_t>(destWidth);
	const int32_t newHeight = static_cast<int32_t>(destHeight);

	uint8_t* pNewBuffer = pBuffer;

	const float xFactor = srcWidth / static_cast<float>(newWidth);
	const float yFactor = srcHeight / static_cast<float>(newHeight);

	float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
	int32_t xCeil, yCeil, xFloor, yFloor, yNewOffset;

	uint8_t c0, c1, c2, c3, cColor;

	for (int32_t y = 0; y < newHeight; ++y)
	{
		yFractioned = y * yFactor;
		yFloor = static_cast<int32_t>(math<float>::floor(yFractioned));
		yCeil = yFloor + 1;

		yFraction = yFractioned - yFloor;
		oneMinusY = 1.0f - yFraction;

		yNewOffset = y * destBufferWidth;

		yFloor += srcY;
		yCeil += srcY;

		if (yCeil >= height_) {
			yCeil = yFloor;
		}

		for (int32_t x = 0; x < newWidth; ++x)
		{
			xFractioned = x * xFactor;
			xFloor = (int32_t)math<float>::floor(xFractioned);
			xCeil = xFloor + 1;

			xFraction = xFractioned - xFloor;
			oneMinusX = 1.0f - xFraction;

			xFloor += srcY;
			xCeil += srcY;

			if (xCeil >= width_) {
				xCeil = xFloor;
			}

			c0 = pBuffer_[yFloor * width_ + xFloor];
			c1 = pBuffer_[yFloor * width_ + xCeil];
			c2 = pBuffer_[yCeil * width_ + xFloor];
			c3 = pBuffer_[yCeil * width_ + xCeil];

			fR0 = (oneMinusX * c0 + xFraction * c1);
			fR1 = (oneMinusX * c2 + xFraction * c3);

			cColor = static_cast<uint8_t>((oneMinusY * fR0) + (yFraction * fR1));

			pNewBuffer[yNewOffset + x + destX] = safe_static_cast<uint8_t, int32_t>(0xffffff | (cColor << 24));
		}
	}

	return true;
}

bool XGlyphBitmap::BlitTo8(uint8_t* pBuffer, 
	int32_t srcX, int32_t srcY, 
	int32_t srcWidth, int32_t srcHeight, 
	int32_t destX, int32_t destY, 
	int32_t destWidth)
{
	int32_t ySrcOffset, yDestOffset;

	for (int32_t y = 0; y < srcHeight; y++)
	{
		ySrcOffset = (srcY + y) * width_;
		yDestOffset = (destY + y) * destWidth;

		for (int32_t x = 0; x < srcWidth; x++)
		{
			pBuffer[yDestOffset + destX + x] = pBuffer_[ySrcOffset + srcX + x];
		}
	}

	return true;
}



bool XGlyphBitmap::BlitTo32(uint32_t* pBuffer,
	int32_t srcX, int32_t srcY,
	int32_t srcWidth, int32_t srcHeight,
	int32_t destX, int32_t destY,
	int32_t destWidth)
{
	int32_t ySrcOffset, yDestOffset;
	char cColor;

	for (int32_t y = 0; y < srcHeight; y++)
	{
		ySrcOffset = (srcY + y) * width_;
		yDestOffset = (destY + y) * destWidth;

		for (int32_t x = 0; x < srcWidth; x++)
		{
			cColor = pBuffer_[ySrcOffset + srcX + x];

			// solid RGB
			pBuffer[yDestOffset + destX + x] = (cColor << 24) | (255 << 16) | (255 << 8) | 255;
		}
	}

	return true;
}




X_NAMESPACE_END