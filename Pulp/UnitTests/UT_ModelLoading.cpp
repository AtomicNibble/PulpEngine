#include "stdafx.h"

#include <IModel.h>

X_USING_NAMESPACE;

using namespace core;

#if 0 // ifdef X_LIB 

TEST(Model, Load)
{
	model::ModelLoader loader;
	model::XModel model;
	bool model_load_success;

	{
		model_load_success = loader.LoadModel(model, "models/default.model");
	}

	EXPECT_TRUE(model_load_success);
}

#else

TEST(Model, Load)
{
	X_WARNING("Model", "skipping model load test in dynamic link build.");

}

#endif // X_LIB