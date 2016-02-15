#pragma once


#include <String\Path.h>

#include "RawModelTypes.h"

#include <IModel.h>


X_NAMESPACE_DECLARE(core,
class XLexer
);

X_NAMESPACE_BEGIN(model)

namespace RawModel
{

	class Model
	{
		typedef core::FixedArray<Lod, model::MODEL_MAX_LODS> LodArr;
		typedef core::Array<Bone> BoneArr;

		static const int32_t VERSION;

	public:
		Model(core::MemoryArenaBase* arena);
		~Model() = default;

		void Clear(void);

		bool LoadRawModel(core::Path<char>& path);
		bool LoadRawModel(core::Path<wchar_t>& path);

		bool SaveRawModel(core::Path<char>& path);
		bool SaveRawModel(core::Path<wchar_t>& path);

		size_t totalMeshes(void) const;

	private:
		bool ParseRawModel(core::XLexer& lex);
		bool ReadBones(core::XLexer& lex, int32_t numBones);
		bool ReadLods(core::XLexer& lex, int32_t numLods);
		bool ReadMesh(core::XLexer& lex, Mesh& mesh);
		bool ReadMaterial(core::XLexer& lex, Material& mat);
		bool ReadMaterialCol(core::XLexer& lex, const char* pName, Color& col);

		bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut);

		bool WriteBones(core::XFile* f) const;
		bool WriteLods(core::XFile* f) const;
		bool WriteMesh(core::XFile* f, const Mesh& mesh) const;
		bool WriteMaterials(core::XFile* f, const Material& mat) const;
		
		
	protected:
		core::MemoryArenaBase* arena_;

		BoneArr bones_;
		LodArr lods_;
	};

} // namespace RawModel


X_NAMESPACE_END