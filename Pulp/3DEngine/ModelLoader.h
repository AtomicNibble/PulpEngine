#pragma once

#ifndef _X_MODEL_LOADER_H_
#define _X_MODEL_LOADER_H_

#include "IFileSys.h"
#include "IModel.h"
#include "XModel.h"

#include "EngineBase.h"

X_NAMESPACE_BEGIN(model)


//
// Loads a mesh into a XMesh
//
//	the data is:
//	
//	||||| HEADER |||||

//  ||||| tag name IDX |||||
//  ||||| Bone Tree    |||||
//  ||||| Bone angles  |||||
//  ||||| Bone pos     |||||
//	
//
//  ||||| All MeshHeaders   |||||
//  ||||| All the verts   |||||
//  ||||| All the faces   |||||
//
// I want to basically just check the header.
// then be able to calculate the total buffer size.
// load it and set the data pointers.
//


class ModelLoader : public engine::XEngineBase
{
public:
	ModelLoader();
	~ModelLoader();

	bool LoadModel(XModel& model, const char* name);
	bool LoadModel(XModel& model, core::XFile* file);


private:
	bool ReadHeader(core::XFile* file);

private:

	ModelHeader header_;
};


X_NAMESPACE_END

#endif // !_X_MODEL_LOADER_H_


