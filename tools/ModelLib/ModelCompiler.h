#pragma once

#include <String\Path.h>

X_NAMESPACE_DECLARE(model,
	namespace RawModel {
		class Model;
	}
);

X_NAMESPACE_BEGIN(model)

class ModelCompiler
{
public:
	ModelCompiler() = default;
	~ModelCompiler() = default;


	bool CompileModel(core::Path<char>& path, const RawModel::Model& model);

private:

};


X_NAMESPACE_END
