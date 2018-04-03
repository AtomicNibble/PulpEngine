#include "stdafx.h"
#include "FxUtil.h"

#include <String\StringRange.h>
#include <String\StringTokenizer.h>

#include <Hashing\Fnva1Hash.h>

using namespace core::Hash::Literals;

X_NAMESPACE_BEGIN(engine)

namespace fx
{
	namespace Util
	{
		StageType::Enum TypeFromStr(const char* pBegin, const char* pEnd)
		{
			static_assert(StageType::ENUM_COUNT == 7, "Added additional types? this code needs updating.");

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
				case "PlayFX"_fnv1a:
					return StageType::PlayFX;
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

		const char* FlagStrFromFlags(StageFlags flags, FlagStr& str)
		{
			str.clear();

			for (int32_t i = 0; i < StageFlags::FLAGS_COUNT; i++)
			{
				auto flag = static_cast<StageFlag::Enum>(1 << i);
				if (flags.IsSet(flag))
				{
					if (str.isNotEmpty()) {
						str.append(" ");
					}
					str.append(StageFlag::ToString(flag));
				}
			}

			return str.c_str();
		}

		StageFlags FlagsFromStr(const char* pBegin, const char* pEnd)
		{
			core::StringRange<char> token(nullptr, nullptr);
			core::StringTokenizer<char> tokens(pBegin, pEnd, ' ');

			StageFlags flags;

			while (tokens.ExtractToken(token))
			{
				static_assert(StageFlag::FLAGS_COUNT == 14, "Added more flags? this needs updating");

				switch (core::Hash::Fnv1aHash(token.GetStart(), token.GetLength()))
				{
					case "Looping"_fnv1a:
						flags.Set(StageFlag::Looping);
						break;
					case "RandGraphCol"_fnv1a:
						flags.Set(StageFlag::RandGraphCol);
						break;
					case "RandGraphAlpha"_fnv1a:
						flags.Set(StageFlag::RandGraphAlpha);
						break;
					case "RandGraphSize0"_fnv1a:
						flags.Set(StageFlag::RandGraphSize0);
						break;
					case "RandGraphSize1"_fnv1a:
						flags.Set(StageFlag::RandGraphSize1);
						break;
					case "RandGraphVel0"_fnv1a:
						flags.Set(StageFlag::RandGraphVel0);
						break;
					case "RandGraphVel1"_fnv1a:
						flags.Set(StageFlag::RandGraphVel1);
						break;
					case "RandGraphRot"_fnv1a:
						flags.Set(StageFlag::RandGraphRot);
						break;
					case "RelativeVel0"_fnv1a:
						flags.Set(StageFlag::RelativeVel0);
						break;
					case "RelativeVel1"_fnv1a:
						flags.Set(StageFlag::RelativeVel1);
						break;
					case "RelativeOrigin"_fnv1a:
						flags.Set(StageFlag::RelativeOrigin);
						break;
					case "NonUniformScale"_fnv1a:
						flags.Set(StageFlag::NonUniformScale);
						break;
					case "SpawnSphere"_fnv1a:
						flags.Set(StageFlag::SpawnSphere);
						break;
					case "SpawnCylindrical"_fnv1a:
						flags.Set(StageFlag::SpawnCylindrical);
						break;
					default:
						X_ERROR("Fx", "Unknown flag: \"%.*s\"", token.GetLength(), token.GetStart());
						break;
				}
			}
		
			return flags;
		}


	} // namespace Util
} // namespace fx

X_NAMESPACE_END
