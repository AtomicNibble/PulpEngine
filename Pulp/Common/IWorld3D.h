#pragma once

#include <ILevel.h>


X_NAMESPACE_BEGIN(engine)


struct RenderEntDesc
{
	model::XModel* pModel;

	Transformf trans;
};

struct IRenderEnt
{


};

struct IWorld3D
{
	virtual ~IWorld3D() {}


	virtual bool loadNodes(const level::FileHeader& fileHdr, level::StringTable& strTable, uint8_t* pData) X_ABSTRACT;

	virtual IRenderEnt* addRenderEnt(RenderEntDesc& ent) X_ABSTRACT;
};



X_NAMESPACE_END