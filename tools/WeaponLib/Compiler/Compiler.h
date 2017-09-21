#pragma once


#include <Containers\ByteStream.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(game)

namespace weapon
{

	class WeaponCompiler
	{
		template<size_t N>
		using StringArr = std::array<core::string, N>;

	public:
		WeaponCompiler();
		~WeaponCompiler();


		bool loadFromJson(core::string& str);
		bool writeToFile(core::XFile* pFile) const;

	private:
		template<typename Enum, typename SlotType, size_t SlotNum, typename Fn>
		static bool parseEnum(core::json::Document& d, core::json::Type expectedType, const char* pPrefix,
			std::array<SlotType, SlotNum>& slots, Fn callBack);

		template<typename SlotEnum, size_t Num>
		static bool writeSlots(const StringArr<Num>& values,
			WeaponHdr::SlotArr<Num>& slotsOut, core::ByteStream& stream);

		template<typename FlagClass, size_t Num>
		static bool processFlagGroup(core::json::Document& d, FlagClass& flags,
			const std::array<std::pair<const char*, typename FlagClass::Enum>, Num>& flagValues);

	private:
		int32_t damageMin_;
		int32_t damageMax_;
		int32_t damageMinRange_;
		int32_t damageMaxRange_;
		int32_t damageMelee_;

		WeaponClass::Enum wpnClass_;
		InventoryType::Enum invType_;
		FireType::Enum fireType_;
		AmmoCounterStyle::Enum ammoCounterStyle_;

		WeaponFlags flags_;

		StringArr<ModelSlot::ENUM_COUNT> modelSlots_;
		StringArr<AnimSlot::ENUM_COUNT> animSlots_;
		StringArr<SoundSlot::ENUM_COUNT> sndSlots_;
		StringArr<IconSlot::ENUM_COUNT> iconSlots_;

		WeaponHdr::FloatArr<StateTimer::ENUM_COUNT> stateTimers_;
	};

}

X_NAMESPACE_END