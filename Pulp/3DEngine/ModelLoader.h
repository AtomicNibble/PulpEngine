#pragma once

#ifndef _X_MODEL_LOADER_H_
#define _X_MODEL_LOADER_H_

#include "IFileSys.h"
#include "IModel.h"
#include "XModel.h"

#include "EngineBase.h"

X_NAMESPACE_BEGIN(model)




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


