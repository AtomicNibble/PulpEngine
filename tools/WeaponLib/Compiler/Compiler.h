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

};

X_NAMESPACE_END