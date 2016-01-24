#pragma once


#include <String\Path.h>

X_NAMESPACE_DECLARE(core,
class XLexer
);

X_NAMESPACE_BEGIN(model)

namespace RawModel
{

	class Model
	{
		typedef core::FixedArray<Lod, 4> LodArr;
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

	private:
		bool ParseRawModel(core::XLexer& lex);
		bool ReadBones(core::XLexer& lex, int32_t numBones);
		bool ReadLods(core::XLexer& lex, int32_t numLods);
		bool ReadMesh(core::XLexer& lex, Mesh& mesh);

		bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut);

		bool WriteBones(FILE* f) const;
		bool WriteLods(FILE* f) const;
		bool WriteMesh(FILE* f, const Mesh& mesh) const;
		bool WriteMaterials(FILE* f, const Material& mat) const;
		
		
	private:
		core::MemoryArenaBase* arena_;

		BoneArr bones_;
		LodArr lods_;
	};

} // namespace RawModel


X_NAMESPACE_END