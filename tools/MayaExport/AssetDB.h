#pragma once

#include <maya\MPxCommand.h>


class AssetDB
{
public:
	AssetDB();
	~AssetDB();

private:

};


class AssetDBCmd : public MPxCommand
{
public:
	AssetDBCmd();
	~AssetDBCmd();

	virtual MStatus doIt(const MArgList &args) X_OVERRIDE;

	static void* creator(void);
	static MSyntax newSyntax(void);

private:

};