#pragma once


X_NAMESPACE_BEGIN(game)


class WeaponLib : public IWeaponLib
{
public:
	WeaponLib();
	~WeaponLib() X_OVERRIDE;

	virtual const char* getOutExtension(void) const X_OVERRIDE;

	virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;
};


X_NAMESPACE_END