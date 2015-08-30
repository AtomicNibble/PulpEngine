#include "stdafx.h"
#include "XGlyphBitmap.h"


X_NAMESPACE_BEGIN(font)

XGlyphBitmap::XGlyphBitmap() :
	pBuffer_(nullptr),
	width_(0),
	height_(0)
{

}

XGlyphBitmap::XGlyphBitmap(int width, int height) :
pBuffer_(nullptr),
width_(0),
height_(0)
{
	Create(width, height);
}



XGlyphBitmap::~XGlyphBitmap()
{
}


bool XGlyphBitmap::Create(int width, int height)
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


void XGlyphBitmap::Release()
{
	X_DELETE_ARRAY(pBuffer_, g_fontArena);
	
	pBuffer_ = nullptr;
	width_ = height_ = 0;
}

void XGlyphBitmap::Blur(int iterations)
{
	int cSum;
	int yOffset;
	int yupOffset;
	int ydownOffset;
	int i, x, y;

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

				pBuffer_[yOffset + x] = safe_static_cast<uint8_t, int>(cSum >> 2);
			}
		}
	}
}

bool XGlyphBitmap::Scale(float scaleX, float scaleY)
{
	// anything to scale?
	if (!pBuffer_)
		return false;

	// Scale changed?
	Vec2f scale(scaleX, scaleY);
	Vec2f sameScale(1.f, 1.f);
	if (sameScale.compare(scale, EPSILON_VALUEf))
		return true;

	// new buffer
	int newWidth = (int)(width_ * scaleX);
	int newHeight = (int)(height_ * scaleY);

	uint8_t* pNewBuffer = X_NEW_ARRAY(uint8_t, newWidth * newHeight, g_fontArena, "ScaleBuffer");

	if (!pNewBuffer)
		return false;

	float xFactor = width_ / (float)newWidth;
	float yFactor = height_ / (float)newHeight;

	float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
	int xCeil, yCeil, xFloor, yFloor, yNewOffset;

	unsigned char c0, c1, c2, c3;

	for (int y = 0; y < newHeight; ++y)
	{
		yFractioned = y * yFactor;
		yFloor = (int)math<float>::floor(yFractioned);
		yCeil = yFloor + 1;

		if (yCeil >= height_) {
			yCeil = yFloor;
		}

		yFraction = yFractioned - yFloor;
		oneMinusY = 1.0f - yFraction;

		yNewOffset = y * newWidth;

		for (int x = 0; x < newWidth; ++x)
		{
			xFractioned = x * xFactor;
			xFloor = (int)math<float>::floor(xFractioned);
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

			pNewBuffer[yNewOffset + x] = (unsigned char)((oneMinusY * fR0) + (yFraction * fR1));
		}
	}

	// update members
	width_ = newWidth;
	height_ = newHeight;

	X_DELETE_ARRAY(pBuffer_, g_fontArena);
	pBuffer_ = pNewBuffer;

	return true;
}

void XGlyphBitmap::Clear()
{
	if (pBuffer_)
		memset(pBuffer_, 0, width_ * height_);
}


//-------------------------------------------------------------------------------------------------
bool XGlyphBitmap::BlitScaledTo8(uint8_t* pBuffer, 
	int iSrcX, int iSrcY, 
	int iSrcWidth, int iSrcHeight, 
	int iDestX, int iDestY, 
	int iDestWidth, int iDestHeight, 
	int iDestBufferWidth)
{
	X_UNUSED(iDestY);
	X_UNUSED(iSrcX);

	int newWidth = (int)iDestWidth;
	int newHeight = (int)iDestHeight;

	uint8_t* pNewBuffer = pBuffer;

	float xFactor = iSrcWidth / (float)newWidth;
	float yFactor = iSrcHeight / (float)newHeight;

	float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
	int xCeil, yCeil, xFloor, yFloor, yNewOffset;

	unsigned char c0, c1, c2, c3;

	for (int y = 0; y < newHeight; ++y)
	{
		yFractioned = y * yFactor;
		yFloor = (int)math<float>::floor(yFractioned);
		yCeil = yFloor + 1;

		yFraction = yFractioned - yFloor;
		oneMinusY = 1.0f - yFraction;

		yNewOffset = y * iDestBufferWidth;

		yFloor += iSrcY;
		yCeil += iSrcY;

		if (yCeil >= height_) {
			yCeil = yFloor;
		}

		for (int x = 0; x < newWidth; ++x)
		{
			xFractioned = x * xFactor;
			xFloor = (int)math<float>::floor(xFractioned);
			xCeil = xFloor + 1;

			xFraction = xFractioned - xFloor;
			oneMinusX = 1.0f - xFraction;

			xFloor += iSrcY;
			xCeil += iSrcY;

			if (xCeil >= width_) {
				xCeil = xFloor;
			}

			c0 = pBuffer_[yFloor * width_ + xFloor];
			c1 = pBuffer_[yFloor * width_ + xCeil];
			c2 = pBuffer_[yCeil * width_ + xFloor];
			c3 = pBuffer_[yCeil * width_ + xCeil];

			fR0 = (oneMinusX * c0 + xFraction * c1);
			fR1 = (oneMinusX * c2 + xFraction * c3);

			pNewBuffer[yNewOffset + x + iDestX] = (uint8_t)((oneMinusY * fR0) + (yFraction * fR1));
		}
	}

	return true;
}

//------------------------------------------------------------------------------------------------- 
bool XGlyphBitmap::BlitScaledTo32(unsigned char *pBuffer,
	int iSrcX, int iSrcY, 
	int iSrcWidth, int iSrcHeight, 
	int iDestX, int iDestY, 
	int iDestWidth, int iDestHeight, 
	int iDestBufferWidth)
{
	X_UNUSED(iDestY);
	X_UNUSED(iSrcX);

	int newWidth = (int)iDestWidth;
	int newHeight = (int)iDestHeight;

	uint8_t* pNewBuffer = pBuffer;

	float xFactor = iSrcWidth / (float)newWidth;
	float yFactor = iSrcHeight / (float)newHeight;

	float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
	int xCeil, yCeil, xFloor, yFloor, yNewOffset;

	unsigned char c0, c1, c2, c3, cColor;

	for (int y = 0; y < newHeight; ++y)
	{
		yFractioned = y * yFactor;
		yFloor = (int)math<float>::floor(yFractioned);
		yCeil = yFloor + 1;

		yFraction = yFractioned - yFloor;
		oneMinusY = 1.0f - yFraction;

		yNewOffset = y * iDestBufferWidth;

		yFloor += iSrcY;
		yCeil += iSrcY;

		if (yCeil >= height_) {
			yCeil = yFloor;
		}

		for (int x = 0; x < newWidth; ++x)
		{
			xFractioned = x * xFactor;
			xFloor = (int)math<float>::floor(xFractioned);
			xCeil = xFloor + 1;

			xFraction = xFractioned - xFloor;
			oneMinusX = 1.0f - xFraction;

			xFloor += iSrcY;
			xCeil += iSrcY;

			if (xCeil >= width_) {
				xCeil = xFloor;
			}

			c0 = pBuffer_[yFloor * width_ + xFloor];
			c1 = pBuffer_[yFloor * width_ + xCeil];
			c2 = pBuffer_[yCeil * width_ + xFloor];
			c3 = pBuffer_[yCeil * width_ + xCeil];

			fR0 = (oneMinusX * c0 + xFraction * c1);
			fR1 = (oneMinusX * c2 + xFraction * c3);

			cColor = (unsigned char)((oneMinusY * fR0) + (yFraction * fR1));

			pNewBuffer[yNewOffset + x + iDestX] = safe_static_cast<uint8_t, int>(0xffffff | (cColor << 24));
		}
	}

	return true;
}

bool XGlyphBitmap::BlitTo8(uint8_t* pBuffer, 
	int iSrcX, int iSrcY, 
	int iSrcWidth, int iSrcHeight, 
	int iDestX, int iDestY, 
	int iDestWidth)
{
	int ySrcOffset, yDestOffset;

	for (int y = 0; y < iSrcHeight; y++)
	{
		ySrcOffset = (iSrcY + y) * width_;
		yDestOffset = (iDestY + y) * iDestWidth;

		for (int x = 0; x < iSrcWidth; x++)
		{
			pBuffer[yDestOffset + iDestX + x] = pBuffer_[ySrcOffset + iSrcX + x];
		}
	}

	return true;
}



bool XGlyphBitmap::BlitTo32(uint32_t* pBuffer,
	int iSrcX, int iSrcY,
	int iSrcWidth, int iSrcHeight,
	int iDestX, int iDestY,
	int iDestWidth)
{
	int ySrcOffset, yDestOffset;
	char cColor;

	for (int y = 0; y < iSrcHeight; y++)
	{
		ySrcOffset = (iSrcY + y) * width_;
		yDestOffset = (iDestY + y) * iDestWidth;

		for (int x = 0; x < iSrcWidth; x++)
		{
			cColor = pBuffer_[ySrcOffset + iSrcX + x];

			// solid RGB
			pBuffer[yDestOffset + iDestX + x] = (cColor << 24) | (255 << 16) | (255 << 8) | 255;
		}
	}

	return true;
}




X_NAMESPACE_END