#include "stdafx.h"
#include "AssetDB.h"


#include <maya\MSyntax.h>



AssetDB::AssetDB()
{


}

AssetDB::~AssetDB()
{

}



// ------------------------------------------------------------------

AssetDBCmd::AssetDBCmd()
{
}

AssetDBCmd::~AssetDBCmd()
{
}

MStatus AssetDBCmd::doIt(const MArgList &args)
{
	X_UNUSED(args);


	return MS::kSuccess;
}

void* AssetDBCmd::creator(void)
{
	return new AssetDBCmd;
}

MSyntax AssetDBCmd::newSyntax(void)
{
	MSyntax syntax;


	return syntax;
}
