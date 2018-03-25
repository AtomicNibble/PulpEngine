#include "stdafx.h"
#include "FxUtil.h"

#include <Hashing\Fnva1Hash.h>

using namespace core::Hash::Literals;

X_NAMESPACE_BEGIN(engine)

namespace fx
{
	namespace Util
	{
		StageType::Enum TypeFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(StageType::ENUM_COUNT == 6, "Added additional types? this code needs updating.");

			const size_t len = (pEnd - pBegin);

			switch (core::Hash::Fnv1aHash(pBegin, len))
			{
				case "BillboardSprite"_fnv1a:
					return StageType::BillboardSprite;
				case "OrientedSprite"_fnv1a:
					return StageType::OrientedSprite;
				case "RotatedSprite"_fnv1a:
					return StageType::RotatedSprite;
				case "Tail"_fnv1a:
					return StageType::Tail;
				case "Line"_fnv1a:
					return StageType::Line;
				case "Sound"_fnv1a:
					return StageType::Sound;
				default:
					X_ERROR("Fx", "Unknown type: '%.*s' (case-sen)", len, pBegin);
					return StageType::BillboardSprite;
			}
		}

		RelativeTo::Enum RelativeToFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(RelativeTo::ENUM_COUNT == 2, "Added additional types? this code needs updating.");

			const size_t len = (pEnd - pBegin);

			switch (core::Hash::Fnv1aHash(pBegin, len))
			{
				case "Spawn"_fnv1a:
					return RelativeTo::Spawn;
				case "Now"_fnv1a:
					return RelativeTo::Now;
				default:
					X_ERROR("Fx", "Unknown RelativeTo: '%.*s' (case-sen)", len, pBegin);
					return RelativeTo::Spawn;
			}
		}


	} // namespace Util
} // namespace fx

X_NAMESPACE_END
