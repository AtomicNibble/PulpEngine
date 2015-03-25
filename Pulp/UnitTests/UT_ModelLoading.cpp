#include "stdafx.h"
#include <gtest\gtest.h>
#include "Profiler.h"

#include <IModel.h>
#include "../3DEngine/ModelLoader.h"

X_USING_NAMESPACE;

using namespace core;

#ifdef X_LIB

TEST(Model, Load)
{
	model::ModelLoader loader;
	model::XModel model;
	bool model_load_success;

	{
		UnitTests::ScopeProfiler profile("ModelLoader");

		model_load_success = loader.LoadModel(model, "core_assets/models/default.model");
	}

	EXPECT_TRUE(model_load_success);
}

#else

TEST(Model, Load)
{
	X_WARNING("Model", "skipping model load test in dynamic link build.");

}

#endif // X_LIB