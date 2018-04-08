#include <EngineCommon.h>
#include "crc32.h"

#include "Util\PointerUtil.h"

namespace
{
    static const uint32_t GF2_DIM = 32;

    uint32_t gf2_matrix_times(uint32_t* mat, uint32_t vec)
    {
        unsigned long sum = 0;
        while (vec) {
            if (vec & 1) {
                sum ^= *mat;
            }
            vec >>= 1;
            mat++;
        }
        return sum;
    }

    void gf2_matrix_square(uint32_t* square, uint32_t* mat)
    {
        int n;
        for (n = 0; n < GF2_DIM; n++) {
            square[n] = gf2_matrix_times(mat, mat[n]);
        }
    }

} // namespace

X_NAMESPACE_BEGIN(core)

uint32_t Crc32::zeroLengthCrc32(void)
{
    return Finish(Begin());
}

uint32_t Crc32::Combine(const uint32_t lhs, const uint32_t rhs,
    const uint32_t rhs_length)
{
    int n;
    uint32_t row;
    uint32_t even[GF2_DIM]; // even-power-of-two zeros operator
    uint32_t odd[GF2_DIM];  // odd-power-of-two zeros operator
    uint32_t crc1, crc2, len2;

    crc1 = lhs;
    crc2 = rhs;
    len2 = rhs_length;

    // degenerate case (also disallow negative lengths)
    if (len2 <= 0) {
        return crc1;
    }

    // put operator for one zero bit in odd
    odd[0] = CRC32_POLY_NORMAL; // CRC-32 polynomial
    row = 1;
    for (n = 1; n < GF2_DIM; n++) {
        odd[n] = row;
        row <<= 1;
    }

    // put operator for two zero bits in even
    gf2_matrix_square(even, odd);

    // put operator for four zero bits in odd
    gf2_matrix_square(odd, even);

    // apply len2 zeros to crc1 (first square will put the operator for one
    // zero byte, eight zero bits, in even)
    do {
        // apply zeros operator for this bit of len2
        gf2_matrix_square(even, odd);
        if (len2 & 1) {
            crc1 = gf2_matrix_times(even, crc1);
        }
        len2 >>= 1;

        // if no more bits set, then done
        if (len2 == 0) {
            break;
        }

        // another iteration of the loop with odd and even swapped
        gf2_matrix_square(odd, even);
        if (len2 & 1) {
            crc1 = gf2_matrix_times(odd, crc1);
        }
        len2 >>= 1;

        // if no more bits set, then done
    } while (len2 != 0);

    // return combined crc
    crc1 ^= crc2;
    return crc1;
}

void Crc32::buildTable(void)
{
    uint32_t i, j;
    for (i = 0; i <= 0xFF; i++) {
        uint32_t crc = i;
        for (j = 0; j < 8; j++)
            crc = (crc >> 1) ^ ((crc & 1) * CRC32_POLY_NORMAL);
        crc32_table_[0][i] = crc;
    }
    for (i = 0; i <= 0xFF; i++) {
        // for Slicing-by-4 and Slicing-by-8
        crc32_table_[1][i] = (crc32_table_[0][i] >> 8) ^ crc32_table_[0][crc32_table_[0][i] & 0xFF];
        crc32_table_[2][i] = (crc32_table_[1][i] >> 8) ^ crc32_table_[0][crc32_table_[1][i] & 0xFF];
        crc32_table_[3][i] = (crc32_table_[2][i] >> 8) ^ crc32_table_[0][crc32_table_[2][i] & 0xFF];
        // only Slicing-by-8
        crc32_table_[4][i] = (crc32_table_[3][i] >> 8) ^ crc32_table_[0][crc32_table_[3][i] & 0xFF];
        crc32_table_[5][i] = (crc32_table_[4][i] >> 8) ^ crc32_table_[0][crc32_table_[4][i] & 0xFF];
        crc32_table_[6][i] = (crc32_table_[5][i] >> 8) ^ crc32_table_[0][crc32_table_[5][i] & 0xFF];
        crc32_table_[7][i] = (crc32_table_[6][i] >> 8) ^ crc32_table_[0][crc32_table_[6][i] & 0xFF];
    }
}

uint32_t Crc32::Update(const void* data, size_t size, uint32_t& crcvalue) const
{
    size_t len;
    const uint8_t* buf8;
    const uint32_t* buf32;
    uint32_t crc = crcvalue;

    len = size;
    buf8 = reinterpret_cast<const uint8_t*>(data);

    while (len && !core::pointerUtil::IsAligned(buf8, 8, 0)) {
        crc = (crc >> 8) ^ crc32_table_[0][(crc & 0xFF) ^ *buf8++];
        len--;
    }

    buf32 = reinterpret_cast<const uint32_t*>(buf8);

    while (len >= 8) {
        uint32_t one = *buf32++ ^ crc;
        uint32_t two = *buf32++;
        crc = crc32_table_[7][one & 0xFF] ^ crc32_table_[6][(one >> 8) & 0xFF] ^ crc32_table_[5][(one >> 16) & 0xFF] ^ crc32_table_[4][one >> 24] ^ crc32_table_[3][two & 0xFF] ^ crc32_table_[2][(two >> 8) & 0xFF] ^ crc32_table_[1][(two >> 16) & 0xFF] ^ crc32_table_[0][two >> 24];
        len -= 8;
    }

    if (len) {
        buf8 = reinterpret_cast<const uint8_t*>(buf32);
        while (len--) {
            crc = (crc >> 8) ^ crc32_table_[0][(crc & 0xFF) ^ *buf8++];
        }
    }

    crcvalue = crc;
    return crcvalue;
}

uint32_t Crc32::UpdateLowerCase(const char* text, size_t size, uint32_t& crcvalue) const
{
    size_t len;
    const uint8_t* buf8;
    const uint32_t* buf32;
    uint32_t crc = crcvalue;

    len = size;
    buf8 = reinterpret_cast<const uint8_t*>(text);

    while (len && !core::pointerUtil::IsAligned(buf8, 8, 0)) {
        crc = (crc >> 8) ^ crc32_table_[0][(crc & 0xFF) ^ ToLower(*buf8++)];
        len--;
    }

    buf32 = reinterpret_cast<const uint32_t*>(buf8);

    while (len >= 8) {
        uint32_t one = ToLower(*buf32++ ^ crc);
        uint32_t two = ToLower(*buf32++);
        crc = crc32_table_[7][one & 0xFF] ^ crc32_table_[6][(one >> 8) & 0xFF] ^ crc32_table_[5][(one >> 16) & 0xFF] ^ crc32_table_[4][one >> 24] ^ crc32_table_[3][two & 0xFF] ^ crc32_table_[2][(two >> 8) & 0xFF] ^ crc32_table_[1][(two >> 16) & 0xFF] ^ crc32_table_[0][two >> 24];
        len -= 8;
    }

    if (len) {
        buf8 = reinterpret_cast<const uint8_t*>(buf32);
        while (len--) {
            crc = (crc >> 8) ^ crc32_table_[0][(crc & 0xFF) ^ ToLower(*buf8++)];
        }
    }

    crcvalue = crc;
    return crcvalue;
}


inline uint32_t Crc32::ToLower(uint32_t c)
{
    union
    {
        uint8_t as_bytes[4];
        uint32_t as_int32;
    };

    as_int32 = c;
    as_bytes[0] = ToLower(as_bytes[0]);
    as_bytes[1] = ToLower(as_bytes[1]);
    as_bytes[2] = ToLower(as_bytes[2]);
    as_bytes[3] = ToLower(as_bytes[3]);
    return as_int32;
}

inline uint8_t Crc32::ToLower(uint8_t c)
{
    return ((((c) >= L'A') && ((c) <= L'Z')) ? ((c)-L'A' + L'a') : (c));
}

inline uint32_t Crc32::Reflect(uint32_t iReflect, const char cChar)
{
    uint32_t iValue = 0;

    // Swap bit 0 for bit 7, bit 1 For bit 6, etc....
    for (int iPos = 1; iPos < (cChar + 1); iPos++) {
        if (iReflect & 1) {
            iValue |= (1 << (cChar - iPos));
        }
        iReflect >>= 1;
    }
    return iValue;
}


X_NAMESPACE_END