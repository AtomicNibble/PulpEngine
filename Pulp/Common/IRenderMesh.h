#pragma once

#ifndef X_RENDER_MODEL_I_H_
#define X_RENDER_MODEL_I_H_

#include <IModel.h>
#include <IShader.h>


X_NAMESPACE_BEGIN(model)

// a mesh that can have both system and video memory.

struct IRenderMesh
{
	virtual ~IRenderMesh() {};

	virtual const int addRef(void) X_ABSTRACT;
	virtual const int release(void) X_ABSTRACT;

	virtual bool render(void) X_ABSTRACT;

	// returns false if no Video memory.
	virtual bool canRender(void) X_ABSTRACT;
	virtual bool uploadToGpu(void) X_ABSTRACT;

	// genral Info
	virtual const char* getName(void) const X_ABSTRACT;
	virtual int getNumVerts(void) const X_ABSTRACT;
	virtual int getNumIndexes(void) const X_ABSTRACT;
	virtual int getNumSubMesh(void) const X_ABSTRACT;

	virtual render::shader::VertexFormat::Enum getVertexFmt(void) const X_ABSTRACT;

	// Mem Info
	virtual int MemoryUsageTotal(void) const X_ABSTRACT;
	virtual int MemoryUsageSys(void) const X_ABSTRACT;
	virtual int MemoryUsageVideo(void) const X_ABSTRACT;


};







X_NAMESPACE_END

#endif // !X_RENDER_MODEL_I_H_