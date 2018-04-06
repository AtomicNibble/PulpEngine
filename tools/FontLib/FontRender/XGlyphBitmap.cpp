#include "stdafx.h"
#include "XGlyphBitmap.h"

X_NAMESPACE_BEGIN(font)

XGlyphBitmap::XGlyphBitmap(core::MemoryArenaBase* arena) :
    buffer_(arena),
    width_(0),
    height_(0)
{
}

XGlyphBitmap::XGlyphBitmap(core::MemoryArenaBase* arena, int32_t width, int32_t height) :
    buffer_(arena),
    width_(0),
    height_(0)
{
    Create(width, height);
}

XGlyphBitmap::~XGlyphBitmap()
{
}

void XGlyphBitmap::Create(int32_t width, int32_t height)
{
    buffer_.resize(width * height);
    width_ = width;
    height_ = height;
}

void XGlyphBitmap::Release(void)
{
    buffer_.free();
    width_ = height_ = 0;
}

void XGlyphBitmap::Blur(int32_t iterations)
{
    int32_t cSum;
    int32_t yOffset;
    int32_t yupOffset;
    int32_t ydownOffset;
    int32_t i, x, y;

    for (i = 0; i < iterations; i++) {
        // loop over each row
        for (y = 0; y < height_; y++) {
            yOffset = y * width_;

            if (y - 1 >= 0) {
                yupOffset = (y - 1) * width_;
            }
            else {
                yupOffset = (y)*width_;
            }

            if (y + 1 < height_) {
                ydownOffset = (y + 1) * width_;
            }
            else {
                ydownOffset = (y)*width_;
            }

            // blur a row
            for (x = 0; x < width_; x++) {
                cSum = buffer_[yupOffset + x] + buffer_[ydownOffset + x];

                if (x - 1 >= 0) {
                    cSum += buffer_[yOffset + x - 1];
                }
                else {
                    cSum += buffer_[yOffset + x];
                }

                if (x + 1 < width_) {
                    cSum += buffer_[yOffset + x + 1];
                }
                else {
                    cSum += buffer_[yOffset + x];
                }

                buffer_[yOffset + x] = safe_static_cast<uint8_t, int32_t>(cSum >> 2);
            }
        }
    }
}

bool XGlyphBitmap::Scale(float scaleX, float scaleY)
{
    // anything to scale?
    if (buffer_.isEmpty()) {
        return false;
    }

    // Scale changed?
    Vec2f scale(scaleX, scaleY);
    Vec2f sameScale(1.f, 1.f);
    if (sameScale.compare(scale, EPSILON_VALUEf)) {
        return true;
    }

    // new buffer
    const int32_t newWidth = static_cast<int32_t>(width_ * scaleX);
    const int32_t newHeight = static_cast<int32_t>(height_ * scaleY);

    DataVec newBuffer(buffer_.getArena(), newWidth * newHeight);

    const float xFactor = width_ / static_cast<float>(newWidth);
    const float yFactor = height_ / static_cast<float>(newHeight);

    float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
    int32_t xCeil, yCeil, xFloor, yFloor, yNewOffset;

    uint8_t c0, c1, c2, c3;

    for (int32_t y = 0; y < newHeight; ++y) {
        yFractioned = y * yFactor;
        yFloor = static_cast<int32_t>(math<float>::floor(yFractioned));
        yCeil = yFloor + 1;

        if (yCeil >= height_) {
            yCeil = yFloor;
        }

        yFraction = yFractioned - yFloor;
        oneMinusY = 1.0f - yFraction;

        yNewOffset = y * newWidth;

        for (int32_t x = 0; x < newWidth; ++x) {
            xFractioned = x * xFactor;
            xFloor = static_cast<int32_t>(math<float>::floor(xFractioned));
            xCeil = xFloor + 1;

            if (xCeil >= width_) {
                xCeil = xFloor;
            }

            xFraction = xFractioned - xFloor;
            oneMinusX = 1.0f - xFraction;

            c0 = buffer_[yFloor * width_ + xFloor];
            c1 = buffer_[yFloor * width_ + xCeil];
            c2 = buffer_[yCeil * width_ + xFloor];
            c3 = buffer_[yCeil * width_ + xCeil];

            fR0 = (oneMinusX * c0 + xFraction * c1);
            fR1 = (oneMinusX * c2 + xFraction * c3);

            newBuffer[yNewOffset + x] = static_cast<uint8_t>((oneMinusY * fR0) + (yFraction * fR1));
        }
    }

    // update members
    width_ = newWidth;
    height_ = newHeight;

    buffer_.swap(newBuffer);
    return true;
}

void XGlyphBitmap::Clear(void)
{
    if (buffer_.isNotEmpty()) {
        std::memset(buffer_.data(), 0, buffer_.size());
    }
}

//-------------------------------------------------------------------------------------------------
bool XGlyphBitmap::BlitScaledTo8(DataVec& destBuffer,
    int32_t srcX, int32_t srcY,
    int32_t srcWidth, int32_t srcHeight,
    int32_t destX, int32_t destY,
    int32_t destWidth, int32_t destHeight,
    int32_t destBufferWidth) const
{
    X_UNUSED(destY);
    X_UNUSED(srcX);

    const int32_t newWidth = static_cast<int32_t>(destWidth);
    const int32_t newHeight = static_cast<int32_t>(destHeight);

    const float xFactor = srcWidth / static_cast<float>(newWidth);
    const float yFactor = srcHeight / static_cast<float>(newHeight);

    float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
    int32_t xCeil, yCeil, xFloor, yFloor, yNewOffset;

    uint8_t c0, c1, c2, c3;

    for (int32_t y = 0; y < newHeight; ++y) {
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

        for (int32_t x = 0; x < newWidth; ++x) {
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

            c0 = buffer_[yFloor * width_ + xFloor];
            c1 = buffer_[yFloor * width_ + xCeil];
            c2 = buffer_[yCeil * width_ + xFloor];
            c3 = buffer_[yCeil * width_ + xCeil];

            fR0 = (oneMinusX * c0 + xFraction * c1);
            fR1 = (oneMinusX * c2 + xFraction * c3);

            destBuffer[yNewOffset + x + destX] = (uint8_t)((oneMinusY * fR0) + (yFraction * fR1));
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
bool XGlyphBitmap::BlitScaledTo32(DataVec& destBuffer,
    int32_t srcX, int32_t srcY,
    int32_t srcWidth, int32_t srcHeight,
    int32_t destX, int32_t destY,
    int32_t destWidth, int32_t destHeight,
    int32_t destBufferWidth) const
{
    X_UNUSED(destY);
    X_UNUSED(srcX);

    const int32_t newWidth = static_cast<int32_t>(destWidth);
    const int32_t newHeight = static_cast<int32_t>(destHeight);

    const float xFactor = srcWidth / static_cast<float>(newWidth);
    const float yFactor = srcHeight / static_cast<float>(newHeight);

    float xFractioned, yFractioned, xFraction, yFraction, oneMinusX, oneMinusY, fR0, fR1;
    int32_t xCeil, yCeil, xFloor, yFloor, yNewOffset;

    uint8_t c0, c1, c2, c3, cColor;

    for (int32_t y = 0; y < newHeight; ++y) {
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

        for (int32_t x = 0; x < newWidth; ++x) {
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

            c0 = buffer_[yFloor * width_ + xFloor];
            c1 = buffer_[yFloor * width_ + xCeil];
            c2 = buffer_[yCeil * width_ + xFloor];
            c3 = buffer_[yCeil * width_ + xCeil];

            fR0 = (oneMinusX * c0 + xFraction * c1);
            fR1 = (oneMinusX * c2 + xFraction * c3);

            cColor = static_cast<uint8_t>((oneMinusY * fR0) + (yFraction * fR1));

            destBuffer[yNewOffset + x + destX] = safe_static_cast<uint8_t, int32_t>(0xffffff | (cColor << 24));
        }
    }

    return true;
}

bool XGlyphBitmap::BlitTo8(uint8_t* pBuffer,
    int32_t srcX, int32_t srcY,
    int32_t srcWidth, int32_t srcHeight,
    int32_t destX, int32_t destY,
    int32_t destWidth) const
{
    int32_t ySrcOffset, yDestOffset;

    X_ASSERT(srcWidth <= width_ && srcHeight <= height_, "Out of range")
    (srcWidth, width_, srcHeight, height_);

    for (int32_t y = 0; y < srcHeight; y++) {
        ySrcOffset = (srcY + y) * width_;
        yDestOffset = (destY + y) * destWidth;

        for (int32_t x = 0; x < srcWidth; x++) {
            pBuffer[yDestOffset + destX + x] = buffer_[ySrcOffset + srcX + x];
        }
    }

    return true;
}

bool XGlyphBitmap::BlitTo24(uint8_t* pBuffer,
    int32_t srcX, int32_t srcY,
    int32_t srcWidth, int32_t srcHeight,
    int32_t destX, int32_t destY,
    int32_t destWidth) const
{
    int32_t ySrcOffset, yDestOffset;

    X_ASSERT(srcWidth <= width_ && srcHeight <= height_, "Out of range")
    (srcWidth, width_, srcHeight, height_);

    for (int32_t y = 0; y < srcHeight; y++) {
        ySrcOffset = (srcY + y) * width_;
        yDestOffset = (destY + y) * (destWidth * 3);

        for (int32_t x = 0; x < srcWidth; x++) {
            uint8_t cColor = buffer_[ySrcOffset + srcX + x];

            uint8_t* pDst = &pBuffer[yDestOffset + ((destX + x) * 3)];
            pDst[0] = cColor;
            pDst[1] = cColor;
            pDst[2] = cColor;
        }
    }

    return true;
}

bool XGlyphBitmap::BlitTo32(uint32_t* pBuffer,
    int32_t srcX, int32_t srcY,
    int32_t srcWidth, int32_t srcHeight,
    int32_t destX, int32_t destY,
    int32_t destWidth) const
{
    int32_t ySrcOffset, yDestOffset;

    X_ASSERT(srcWidth <= width_ && srcHeight <= height_, "Out of range")
    (srcWidth, width_, srcHeight, height_);

    for (int32_t y = 0; y < srcHeight; y++) {
        ySrcOffset = (srcY + y) * width_;
        yDestOffset = (destY + y) * destWidth;

        for (int32_t x = 0; x < srcWidth; x++) {
            uint8_t cColor = buffer_[ySrcOffset + srcX + x];

            // solid RGB
            pBuffer[yDestOffset + destX + x] = (cColor << 24) | (255 << 16) | (255 << 8) | 255;
        }
    }

    return true;
}

X_NAMESPACE_END