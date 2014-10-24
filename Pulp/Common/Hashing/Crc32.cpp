#include <EngineCommon.h>
#include "crc32.h"

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

}

X_NAMESPACE_BEGIN(core)


uint32_t Crc32::Combine(const uint32_t lhs, const uint32_t rhs,
	const uint32_t rhs_length) const
{
	int n;
	uint32_t row;
	uint32_t even[GF2_DIM];    // even-power-of-two zeros operator 
	uint32_t odd[GF2_DIM];     // odd-power-of-two zeros operator 
	uint32_t crc1, crc2, len2;

	crc1 = lhs;
	crc2 = rhs;
	len2 = rhs_length;

	// degenerate case (also disallow negative lengths) 
	if (len2 <= 0) {
		return crc1;
	}

	// put operator for one zero bit in odd 
	odd[0] = CRC32_POLY_NORMAL;   // CRC-32 polynomial 
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
	do 
	{
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


void Crc32::build_table()
{
	// I could change this poly if i didnt want my crc to match
	// a standard crc32
	uint32_t uPolynomial = CRC32_POLY_NORMAL;
	//	uint32_t uCrc;

	//	int i, j;
	/*
	for (i = 0; i < 256; i++) {
	uCrc = i;
	for (j = 8; j > 0; j--) {
	if (uCrc & 1)
	uCrc = (uCrc >> 1) ^ uPolynomial;
	else
	uCrc >>= 1;
	}
	crc32_table[i] = uCrc;
	}*/
	core::zero_object(crc32_table);

	for (int iCodes = 0; iCodes <= 0xFF; iCodes++)
	{
		crc32_table[iCodes] = Reflect(iCodes, 8) << 24;

		for (int iPos = 0; iPos < 8; iPos++)
		{
			crc32_table[iCodes] = (crc32_table[iCodes] << 1)
				^ ((crc32_table[iCodes] & (1 << 31)) ? uPolynomial : 0);
		}

		crc32_table[iCodes] = Reflect(crc32_table[iCodes], 32);
	}

	tableInit_ = true;
}


X_NAMESPACE_END