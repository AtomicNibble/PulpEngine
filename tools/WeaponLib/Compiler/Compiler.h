#pragma once


X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(game)

class WeaponCompiler
{
public:
	WeaponCompiler();
	~WeaponCompiler();


	bool loadFromJson(core::string& str);
	bool writeToFile(core::XFile* pFile) const;

private:
	template<typename FlagClass, size_t Num>
	bool processFlagGroup(core::json::Document& d, FlagClass& flags, 
		const std::array<std::pair<const char*, typename FlagClass::Enum>, Num>& flagValues);

private:
	WeaponClass::Enum wpnClass_;
	InventoryType::Enum invType_;
	FireType::Enum fireType_;
	AmmoCounterStyle::Enum ammoCounterStyle_;

	WeaponFlags flags_;
};

X_NAMESPACE_END