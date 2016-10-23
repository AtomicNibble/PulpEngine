#include "stdafx.h"
#include "Compiler.h"

X_NAMESPACE_BEGIN(engine)


MaterialCompiler::MaterialCompiler()
{

}


bool MaterialCompiler::loadFromJson(core::string& str)
{
	core::json::Document d;
	d.Parse(str.c_str(), str.length());

	// find all the things.





	return false;
}

X_NAMESPACE_END