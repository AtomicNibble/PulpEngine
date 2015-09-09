
/// \ingroup Util
namespace bitUtil
{
	// tell the compiler to use the intrinsic versions of these functions
	#pragma intrinsic(_BitScanReverse)
	#pragma intrinsic(_BitScanReverse64)
	#pragma intrinsic(_BitScanForward)
	#pragma intrinsic(_BitScanForward64)

	/// \ingroup Util
	namespace internal
	{
		/// \brief Base template for types of size N.
		/// \details The individual template specializations take care of working with the correct unsigned type
		/// based on the size of the generic type T. First, this allows for optimizations based on the size of a given
		/// type (counting bits for 16-bit types differs from counting bits for 32-bit types). Second, the implementations can make
		/// sure to only use unsigned types when working with individual bits. Signed types should be avoided for bit
		/// manipulation because some operations like e.g. a right-shift are implementation-defined for signed types
		/// in C++.
		template <size_t N>
		struct Implementation {};

		/// Template specialization for 4-byte types.
		template <>
		struct Implementation<8u>
		{
			template <typename T>
			static inline bool IsBitFlagSet(T value, unsigned int flag)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return (static_cast<uint64_t>(value) & flag) == flag;
			}

			template <typename T>
			static inline T ClearBitFlag(T value, unsigned int flag)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return (static_cast<uint64_t>(value) & (~flag));
			}

			/// Internal function used by bitUtil::RoundUpToMultiple.
			template <typename T>
			static inline uint64_t RoundUpToMultiple(T numToRound, T multipleOf)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
			}

			/// Internal function used by bitUtil::CountBits.
			template <typename T>
			static inline unsigned int CountBits(T value)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				uint32_t v = static_cast<uint32_t>(value) - ((static_cast<uint32_t>(value) >> 1u) & 0x5555555555555555UL);
				v = (v & 0x3333333333333333UL) + ((v >> 2) & 0x3333333333333333UL);
				return (((v + (v >> 4u) & 0xF0F0F0F0F0F0F0FUL) * 0x101010101010101UL) >> 56);
			}

			/// Internal function used by bitUtil::ScanBits.
			template <typename T>
			static inline unsigned int ScanBits(T value)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				unsigned long index = 0;
				const unsigned char result = _BitScanReverse64(&index, static_cast<uint64_t>(value));
				if (result == 0) {
					return NO_BIT_SET;
				}

				return index;
			}

			template <typename T>
			static inline unsigned int ScanBitsForward(T value)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				unsigned long index = 0;
				const unsigned char result = _BitScanForward64(&index, static_cast<uint64_t>(value));
				if (result == 0) {
					return NO_BIT_SET;
				}

				return index;
			}


			/// Internal function used by bitUtil::SetBit.
			template <typename T>
			static inline T SetBit(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return (static_cast<T>(static_cast<uint64_t>(value) | (1ui64 << whichBit)));
			}		
			
			/// Internal function used by bitUtil::ClearBit.
			template <typename T>
			static inline T ClearBit(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return (static_cast<T>(static_cast<uint64_t>(value)& (~(1ui64 << whichBit))));
			}

			/// Internal function used by bitUtil::ReplaceBits.
			template <typename T>
			static inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				const uint64_t mask = ~(((1ui64 << howMany) - 1ui64) << startBit);
				return (static_cast<T>((static_cast<uint64_t>(value) & mask) | (static_cast<uint64_t>(bits) << startBit)));
			}

			/// Internal function used by bitUtil::IsBitSet.
			template <typename T>
			static inline bool IsBitSet(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return ((static_cast<uint64_t>(value) & (1ui64 << whichBit)) != 0);
			}

			template <typename T>
			static inline T NextPowerOfTwo(T v)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				--v;
				v |= v >> 1;
				v |= v >> 2;
				v |= v >> 4;
				v |= v >> 8;
				v |= v >> 16;
				v |= v >> 32;
				++v;
				return v;
			}

			template<typename T>
			static inline bool isSignBitSet(T value)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return IsBitSet<T>(value, 63);
			}

			template<typename T>
			static inline bool isSignBitNotSet(T value)
			{
				static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

				return IsBitSet<T>(value, 63) == false;
			}
			
		};

		/// Template specialization for 4-byte types.
		template <>
		struct Implementation<4u>
		{
			/// Internal function used by bitUtil::IsBitFlagSet.
			template <typename T>
			static inline bool IsBitFlagSet(T value, unsigned int flag)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return (static_cast<uint32_t>(value) & flag) == flag;
			}

			template <typename T>
			static inline T ClearBitFlag(T value, unsigned int flag)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return (static_cast<uint32_t>(value) & (~flag));
			}

			/// Internal function used by bitUtil::IsBitSet.
			template <typename T>
			static inline bool IsBitSet(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return ((static_cast<uint32_t>(value) & (1u << whichBit)) != 0);
			}

			/// Internal function used by bitUtil::SetBit.
			template <typename T>
			static inline T SetBit(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return (static_cast<T>(static_cast<uint32_t>(value) | (1u << whichBit)));
			}

			/// Internal function used by bitUtil::ClearBit.
			template <typename T>
			static inline T ClearBit(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return (static_cast<T>(static_cast<uint32_t>(value) & (~(1u << whichBit))));
			}

			/// Internal function used by bitUtil::ReplaceBits.
			template <typename T>
			static inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				const uint32_t mask = ~(((1u << howMany) - 1u) << startBit);
				return (static_cast<T>((static_cast<uint32_t>(value) & mask) | (static_cast<uint32_t>(bits) << startBit)));
			}

			/// Internal function used by bitUtil::CountBits.
			template <typename T>
			static inline unsigned int CountBits(T value)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				uint32_t v = static_cast<uint32_t>(value) - ((static_cast<uint32_t>(value) >> 1u) & 0x55555555u);
				v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
				return (((v + (v >> 4u) & 0xF0F0F0Fu) * 0x1010101u) >> 24u);
			}

			/// Internal function used by bitUtil::ScanBits.
			template <typename T>
			static inline unsigned int ScanBits(T value)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				unsigned long index = 0;
				const unsigned char result = _BitScanReverse(&index, static_cast<DWORD>(value));
				if (result == 0) {
					return NO_BIT_SET;
				}

				return index;
			}

			template <typename T>
			static inline unsigned int ScanBitsForward(T value)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				unsigned long index = 0;
				const unsigned char result = _BitScanForward(&index, static_cast<DWORD>(value));
				if (result == 0) {
					return NO_BIT_SET;
				}

				return index;
			}


			/// Internal function used by bitUtil::RoundUpToMultiple.
			template <typename T>
			static inline unsigned int RoundUpToMultiple(T numToRound, T multipleOf)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
			}

			template <typename T>
			static inline T NextPowerOfTwo(T v)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				--v;
				v |= v >> 1;
				v |= v >> 2;
				v |= v >> 4;
				v |= v >> 8;
				v |= v >> 16;
				++v;

				return v;
			}

			template<typename T>
			static inline bool isSignBitSet(T value)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return IsBitSet<T>(value, 31);
			}

			template<typename T>
			static inline bool isSignBitNotSet(T value)
			{
				static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

				return IsBitSet<T>(value, 31) == false;
			}
			
		};

		template <>
		struct Implementation<2u>
		{
			/// Internal function used by bitUtil::IsBitSet.
			template <typename T>
			static inline bool IsBitSet(T value, unsigned int whichBit)
			{
				static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

				return ((static_cast<uint16_t>(value) & (1u << whichBit)) != 0);
			}

			template<typename T>
			static inline bool isSignBitSet(T value)
			{
				static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

				return IsBitSet<T>(value, 15);
			}

			template<typename T>
			static inline bool isSignBitNotSet(T value)
			{
				static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

				return IsBitSet<T>(value, 15) == false;
			}
			
		};
	}


	template <typename T>
	inline bool IsBitFlagSet(T value, unsigned int flag)
	{
		return internal::Implementation<sizeof(T)>::IsBitFlagSet(value, flag);
	}


	template <typename T>
	inline T ClearBitFlag(T value, unsigned int flag)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::ClearBitFlag(value, flag);
	}

	template <typename T>
	inline bool IsBitSet(T value, unsigned int whichBit)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::IsBitSet(value, whichBit);
	}

	template <typename T>
	inline bool IsBitNotSet(T value, unsigned int whichBit)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::IsBitSet(value, whichBit) == false;
	}

	template <typename T>
	inline T SetBit(T value, unsigned int whichBit)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::SetBit(value, whichBit);
	}


	template <typename T>
	inline T ClearBit(T value, unsigned int whichBit)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::ClearBit(value, whichBit);
	}


	template <typename T>
	inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::ReplaceBits(value, startBit, howMany, bits);
	}


	template <typename T>
	inline unsigned int CountBits(T value)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::CountBits(value);
	}


	template <typename T>
	inline unsigned int ScanBits(T value)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::ScanBits(value);
	}

	template <typename T>
	inline unsigned int ScanBitsForward(T value)
	{
		// defer the implementation to the correct helper template, based on the size of the type
		return internal::Implementation<sizeof(T)>::ScanBitsForward(value);
	}

	template <typename T>
	inline bool IsPowerOfTwo(T x)
	{
		return (x & (x - 1)) == 0;
	}


	template <typename T>
	inline T NextPowerOfTwo(T v)
	{
		return internal::Implementation<sizeof(T)>::NextPowerOfTwo(v);
	}


	template <typename T>
	inline T RoundUpToMultiple(T numToRound, T multipleOf)
	{
		return internal::Implementation<sizeof(T)>::RoundUpToMultiple(numToRound, multipleOf);
	}


	inline uint32_t RoundDownToMultiple(uint32_t numToRound, uint32_t multipleOf)
	{
		return numToRound & ~(multipleOf - 1);
	}


	// sign util
	template<typename T>
	inline bool isSignBitSet(T value)
	{
		return internal::Implementation<sizeof(T)>::isSignBitSet(value);
	}

	// sign util
	template<typename T>
	inline bool isSignBitNotSet(T value)
	{
		return internal::Implementation<sizeof(T)>::isSignBitNotSet(value);
	}
}
