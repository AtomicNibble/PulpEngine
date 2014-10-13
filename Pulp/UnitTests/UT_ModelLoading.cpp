#include "stdafx.h"
#include <gtest\gtest.h>


#include <IModel.h>
#include "../3DEngine/ModelLoader.h"

X_USING_NAMESPACE;

using namespace core;

class ScopeProfiler
{
public:
	ScopeProfiler(const char* name) : name_(name) {
		start_ = getTime();
	}
	~ScopeProfiler() {
		int64_t elapsed = (getTime() - start_);

		double ms = convertToMs(elapsed);

		X_LOG0(name_, "Operation took %5.4fms", ms);
	}

private:
	const char* name_;
	int64_t start_;

	static double s_frequency;

public:
	static double convertToMs(int64_t elapsed) {
		if (s_frequency == 0.f)
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			s_frequency = 1.0 / double(freq.QuadPart);
			s_frequency *= 1000.0;
		}

		return elapsed * s_frequency;
	}

	static int64_t getTime() {
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);
		return  start.QuadPart;
	}
};

double ScopeProfiler::s_frequency;

#ifdef X_LIB

TEST(Model, Load)
{
	model::ModelLoader loader;
	model::XModel model;
	bool model_load_success;

	{
		ScopeProfiler profile("ModelLoader");

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